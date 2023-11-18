#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QOpenGLDebugLogger>
#include "modeldata.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum class RenderMode
{
    Wireframe,
    Flat,
    Textured
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static MainWindow &instance() { return *_instance; }

    bool showGrid() const;
    bool showOrigin() const;
    bool vertexTicks() const;
    int activeFrame() const;
    int animationFrameRate() const;
    bool animationInterpolated() const;
    int animationStartFrame() const;
    int animationEndFrame() const;
    RenderMode renderMode2D() const;
    bool drawBackfaces2D() const;
    bool smoothNormals2D() const;
    RenderMode renderMode3D() const;
    bool drawBackfaces3D() const;
    bool smoothNormals3D() const;

    QSettings settings;

private:
    Ui::MainWindow *ui;
    ModelData activeModel;

    void openClicked();
    void frameChanged();
    void animationChanged();
    void toggleAnimation();

    static MainWindow *_instance;

public:
    void loadModel(QString path);
};
