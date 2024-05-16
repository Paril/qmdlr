layout(std140) uniform RenderData
{
	mat4 drag3DMatrix;
	mat4 dragUVMatrix;
	int  flags;
};

#define FLAG_DRAG_SELECTED 1
#define FLAG_UV_SELECTED   2
#define FLAG_FACE_MODE     4