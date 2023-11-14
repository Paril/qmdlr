#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <qopenglcontext.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(this->ui->actionOpen, &QAction::triggered, this, &MainWindow::openClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

#include <QFileDialog>

/*
========================================================================

.MD2 triangle model file format

========================================================================
*/

#define IDALIASHEADER		(('2'<<24)+('P'<<16)+('D'<<8)+'I')
#define ALIAS_VERSION	8

#define	MAX_TRIANGLES	4096
#define MAX_VERTS		2048
#define MAX_FRAMES		512
#define MAX_MD2SKINS	32
#define	MAX_SKINNAME	64

struct dstvert_t
{
	short	s;
	short	t;
};

struct dtriangle_t
{
	std::array<short, 3> index_xyz;
	std::array<short, 3> index_st;
};

struct dtrivertx_t
{
	std::array<byte, 3> v;			// scaled byte to fit in frame mins/maxs
	byte	            lightnormalindex;
};

struct daliasframe_t
{
	std::array<float, 3> scale;	// multiply byte verts by this
	std::array<float, 3> translate;	// then add this
	char		         name[16];	// frame name from grabbing
	//dtrivertx_t	verts[1];	// variable sized
};

struct dmdl_t
{
	int			ident;
	int			version;

	int			skinwidth;
	int			skinheight;
	int			framesize;		// byte size of each frame

	int			num_skins;
	int			num_xyz;
	int			num_st;			// greater than num_xyz for seams
	int			num_tris;
	int			num_glcmds;		// dwords in strip/fan command list
	int			num_frames;

	int			ofs_skins;		// each skin is a MAX_SKINNAME string
	int			ofs_st;			// byte offset from start for stverts
	int			ofs_tris;		// offset for dtriangles
	int			ofs_frames;		// offset for first frame
	int			ofs_glcmds;	
	int			ofs_end;		// end of file
};

struct pcx_t
{
    char    manufacturer;
    char    version;
    char    encoding;
    char    bits_per_pixel;
    unsigned short  xmin,ymin,xmax,ymax;
    unsigned short  hres,vres;
    unsigned char   palette[48];
    char    reserved;
    char    color_planes;
    unsigned short  bytes_per_line;
    unsigned short  palette_type;
    char    filler[58];
};

/*
==============
LoadPCX
==============
*/
bool LoadPCX (QDataStream &skinStream, ModelSkin &skin)
{
    pcx_t   pcx;
    int     x, y;
    uint8_t dataByte, runLength;
    byte    *pix;

    //
    // parse the PCX file
    //
	skinStream >> pcx.manufacturer >> pcx.version >> pcx.encoding >> pcx.bits_per_pixel;
	skinStream >> pcx.xmin >> pcx.ymin >> pcx.xmax >> pcx.ymax;
	skinStream >> pcx.hres >> pcx.vres;
	skinStream.skipRawData(sizeof(pcx_t::palette));
	skinStream >> pcx.reserved >> pcx.color_planes;
	skinStream >> pcx.bytes_per_line >> pcx.palette_type;
	skinStream.skipRawData(sizeof(pcx_t::filler));

    if (pcx.manufacturer != 0x0a
        || pcx.version != 5
        || pcx.encoding != 1
        || pcx.bits_per_pixel != 8)
    {
		return false;
    }

	skin.width = pcx.xmax + 1;
	skin.height = pcx.ymax + 1;

	skin.data.resize(skin.width * skin.height * 4);
	pix = skin.data.data();

	byte palette[768];
	skinStream.device()->seek(skinStream.device()->size() - 768);

	skinStream.readRawData(reinterpret_cast<char *>(palette), 768);

	skinStream.device()->seek(sizeof(pcx_t));

    for (y=0 ; y<=pcx.ymax ; y++, pix += (pcx.xmax + 1) * 4)
    {
        for (x=0; x<=pcx.xmax ; )
        {
            skinStream >> dataByte;

            if((dataByte & 0xC0) == 0xC0)
            {
                runLength = dataByte & 0x3F;
                skinStream >> dataByte;
            }
            else
                runLength = 1;

            while(runLength-- > 0)
			{
                pix[(x * 4) + 0] = palette[(dataByte * 3) + 0];
                pix[(x * 4) + 1] = palette[(dataByte * 3) + 1];
                pix[(x * 4) + 2] = palette[(dataByte * 3) + 2];
				pix[(x * 4) + 3] = 0xFF;
				x++;
			}
        }
    }

	return true;
}

ModelData LoadMD2(QString filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::ExistingOnly))
        throw std::runtime_error("bad");

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
	stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

	dmdl_t header;
	stream.readRawData(reinterpret_cast<char *>(&header), sizeof(header));

	ModelData data;

	data.frames.resize(header.num_frames);

	for (auto &frame : data.frames)
		frame.vertices.resize(header.num_xyz);

	file.seek(header.ofs_frames);

	for (auto &frame : data.frames)
	{
		qint64 pos = file.pos();
		daliasframe_t frame_header;
		stream >> frame_header.scale[0] >> frame_header.scale[1] >> frame_header.scale[2];
		pos = file.pos();
		stream >> frame_header.translate[0] >> frame_header.translate[1] >> frame_header.translate[2];
		pos = file.pos();
		stream.readRawData(frame_header.name, sizeof(frame_header.name));
		frame_header.name[sizeof(frame_header.name) - 1] = '\0';

		frame.name = frame_header.name;

		for (auto &vert : frame.vertices)
		{
			dtrivertx_t v;
			stream >> v.v[0] >> v.v[1] >> v.v[2];
			stream >> v.lightnormalindex;

			vert.position = {
				(v.v[0] * frame_header.scale[0]) + frame_header.translate[0],
				(v.v[1] * frame_header.scale[1]) + frame_header.translate[1],
				(v.v[2] * frame_header.scale[2]) + frame_header.translate[2]
			};
		}
	}

	file.seek(header.ofs_st);
	data.texcoords.resize(header.num_st);

	for (auto &st : data.texcoords)
	{
		dstvert_t v;
		stream >> v.s >> v.t;
		st = { (float) v.s / header.skinwidth, (float) v.t / header.skinheight };
	}

	file.seek(header.ofs_tris);
	data.triangles.resize(header.num_tris);

	for (auto &tri : data.triangles)
	{
		dtriangle_t t;
		stream >> t.index_xyz[0] >> t.index_xyz[1] >> t.index_xyz[2];
		stream >> t.index_st[0] >> t.index_st[1] >> t.index_st[2];
		
		std::copy(t.index_xyz.begin(), t.index_xyz.end(), tri.vertices.begin());
		std::copy(t.index_st.begin(), t.index_st.end(), tri.texcoords.begin());
	}

	data.skins.resize(header.num_skins);
	
	file.seek(header.ofs_skins);

	QDir model_dir = QFileInfo(filename).dir();
	QString model_file = model_dir.absolutePath();

	for (auto &skin : data.skins)
	{
		char skin_path[64];
		stream.readRawData(skin_path, sizeof(skin_path));
		skin_path[sizeof(skin_path) - 1] = '\0';

		// try to find the matching PCX file
		QDir skin_dir = model_dir;
		QFileInfo skin_file;

		while (!skin_dir.isRoot())
		{
			skin_file = QFileInfo(skin_dir.filePath(skin_path));

			if (skin_file.exists())
				break;

			skin_dir.cdUp();
		}

		if (!skin_file.exists())
			continue;

		QFile sf = QFile(skin_file.filePath());

		if (!sf.open(QIODevice::ReadOnly | QIODevice::ExistingOnly))
			continue;
		
		QDataStream skinStream(&sf);
		skinStream.setByteOrder(QDataStream::LittleEndian);
		skinStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

		LoadPCX(skinStream, skin);
	}

	return data;
}

void MainWindow::loadModel(QString path)
{
    activeModel = LoadMD2(path);
	this->ui->openGLWidget->setModel(&activeModel);
}

void MainWindow::openClicked(bool toggled)
{
    QFileDialog dlg(this, "Load MD2", "", "*.md2");
    if (dlg.exec() == QFileDialog::Accepted)
		loadModel(dlg.selectedFiles()[0]);
}
