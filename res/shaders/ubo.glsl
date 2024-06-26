layout(std140) uniform RenderData
{
	mat4 drag3DMatrix;
	mat4 dragUVMatrix;
	int  flags;
};

#define FLAG_DRAG_SELECTED 1
#define FLAG_UV_SELECTED   2
#define FLAG_FACE_MODE     4

#define FLAG_SELECTED_FACE      1
#define FLAG_SELECTED_VERTEX    2
#define FLAG_SELECTED_UV        4