/************************************************************************************

Filename    :   OculusRoomModel.cpp
Content     :   Creates a simple room scene from hard-coded geometry
Created     :   October 4, 2012

Copyright   :   Copyright 2012 Oculus, Inc. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*************************************************************************************/

#include "RenderTiny_D3D11_Device.h"

#include "libpng/png.h"

#include "OculusBoard.h"

#include <sstream>
#include <vector>

//-------------------------------------------------------------------------------------
// ***** Room Model

// This model is hard-coded out of axis-aligned solid-colored slabs.
// Room unit dimensions are in meters. Player starts in the middle.
//

enum BuiltinTexture
{
    Tex_None,
    Tex_Checker,
    Tex_Block,
    Tex_Panel,
	Tex_Table,
	Tex_CueBall,
	Tex_Ball1,
	Tex_Ball2,
	Tex_Ball3,
	Tex_Ball4,
	Tex_Ball5,
	Tex_Ball6,
	Tex_Ball7,
	Tex_Ball8,
	Tex_Ball9,
	Tex_Ball10,
	Tex_Ball11,
	Tex_Ball12,
	Tex_Ball13,
	Tex_Ball14,
	Tex_Ball15,
    Tex_Count
};

struct Slab
{
    float x1, y1, z1;
    float x2, y2, z2;
    Color c;
};

struct SlabModel
{
    int   Count;
    const Slab* pSlabs;
    BuiltinTexture tex;
};

Slab FloorSlabs[] =
{
    // Floor
    { -10.0f,  -0.1f,  -20.0f,  10.0f,  0.0f, 20.1f,  Color(128,128,128) }
};

SlabModel Floor = {sizeof(FloorSlabs)/sizeof(Slab), FloorSlabs, Tex_Checker};


Slab CeilingSlabs[] =
{
    { -10.0f,  4.0f,  -20.0f,  10.0f,  4.1f, 20.1f,  Color(128,128,128) }
};

SlabModel Ceiling = {sizeof(FloorSlabs)/sizeof(Slab), CeilingSlabs, Tex_Panel};

Slab RoomSlabs[] =
{
    // Left Wall
    { -10.1f,   0.0f,  -20.0f, -10.0f,  4.0f, 20.0f,  Color(128,128,128) },
    // Back Wall
    { -10.0f,  -0.1f,  -20.1f,  10.0f,  4.0f, -20.0f, Color(128,128,128) },

    // Right Wall
    {  10.0f,  -0.1f,  -20.0f,  10.1f,  4.0f, 20.0f,  Color(128,128,128) },
};

SlabModel Room = {sizeof(RoomSlabs)/sizeof(Slab), RoomSlabs, Tex_Block};

Slab FixtureSlabs[] =
{
    // Right side shelf
    {   9.5f,   0.75f,  3.0f,  10.1f,  2.5f,   3.1f,  Color(128,128,128) }, // Verticals
    {   9.5f,   0.95f,  3.7f,  10.1f,  2.75f,  3.8f,  Color(128,128,128) },
    {   9.5f,   1.20f,  2.5f,  10.1f,  1.30f,  3.8f,  Color(128,128,128) }, // Horizontals
    {   9.5f,   2.00f,  3.0f,  10.1f,  2.10f,  4.2f,  Color(128,128,128) },

    // Right railing    
    {   5.0f,   1.1f,   20.0f,  10.0f,  1.2f,  20.1f, Color(128,128,128) },
    // Bars
    {   9.0f,   1.1f,   20.0f,   9.1f,  0.0f,  20.1f, Color(128,128,128) },
    {   8.0f,   1.1f,   20.0f,   8.1f,  0.0f,  20.1f, Color(128,128,128) },
    {   7.0f,   1.1f,   20.0f,   7.1f,  0.0f,  20.1f, Color(128,128,128) },
    {   6.0f,   1.1f,   20.0f,   6.1f,  0.0f,  20.1f, Color(128,128,128) },
    {   5.0f,   1.1f,   20.0f,   5.1f,  0.0f,  20.1f, Color(128,128,128) },

    // Left railing    
    {  -10.0f,   1.1f, 20.0f,   -5.0f,   1.2f, 20.1f, Color(128,128,128) },
    // Bars
    {  -9.0f,   1.1f,   20.0f,  -9.1f,  0.0f,  20.1f, Color(128,128,128) },
    {  -8.0f,   1.1f,   20.0f,  -8.1f,  0.0f,  20.1f, Color(128,128,128) },
    {  -7.0f,   1.1f,   20.0f,  -7.1f,  0.0f,  20.1f, Color(128,128,128) },
    {  -6.0f,   1.1f,   20.0f,  -6.1f,  0.0f,  20.1f, Color(128,128,128) },
    {  -5.0f,   1.1f,   20.0f,  -5.1f,  0.0f,  20.1f, Color(128,128,128) },

    // Bottom Floor 2
    { -15.0f,  -6.1f,   18.0f,  15.0f, -6.0f, 30.0f,  Color(128,128,128) },
};

SlabModel Fixtures = {sizeof(FixtureSlabs)/sizeof(Slab), FixtureSlabs};

static const float longEnd = 3.7 / 2.;
static const float shortEnd = longEnd / 2.f;
static const float inset = 0.1f;
static const float outset = 0.1f;
static const float height = 0.75f;
static const float rim = 2 * Ball::defaultRadius;

static const Slab TableSlabs[] =
{
    // Table
    {  -shortEnd,          0.0f,    longEnd,   shortEnd,          height,       -longEnd,      Color(75,128,128) }, // Table face
};

static const SlabModel Table = {sizeof(TableSlabs)/sizeof(Slab), TableSlabs, Tex_Table};

static const Slab CushionSlabs[] =
{
    {  -shortEnd - outset, height,  longEnd,  -shortEnd + inset,  height + rim, -longEnd,      Color(128,128,88) }, // Left edge
    {   shortEnd - inset,  height,  longEnd,   shortEnd + outset, height + rim, -longEnd, Color(128,128,88) }, // Right edge
    {  -shortEnd, height,  longEnd - inset,   shortEnd,         height + rim,  longEnd + outset, Color(128,128,88) }, // Front edge
    {  -shortEnd, height, -longEnd - outset,  shortEnd,         height + rim, -longEnd + inset, Color(128,128,88) }, // Back edge
};

static const SlabModel Cushions = {sizeof(CushionSlabs)/sizeof(Slab), CushionSlabs};

Slab PostsSlabs[] = 
{
    // Posts
    {  0,  0.0f, 0.0f,   0.1f, 1.3f, 0.1f, Color(128,128,128) },
    {  0,  0.0f, 0.4f,   0.1f, 1.3f, 0.5f, Color(128,128,128) },
    {  0,  0.0f, 0.8f,   0.1f, 1.3f, 0.9f, Color(128,128,128) },
    {  0,  0.0f, 1.2f,   0.1f, 1.3f, 1.3f, Color(128,128,128) },
    {  0,  0.0f, 1.6f,   0.1f, 1.3f, 1.7f, Color(128,128,128) },
    {  0,  0.0f, 2.0f,   0.1f, 1.3f, 2.1f, Color(128,128,128) },
    {  0,  0.0f, 2.4f,   0.1f, 1.3f, 2.5f, Color(128,128,128) },
    {  0,  0.0f, 2.8f,   0.1f, 1.3f, 2.9f, Color(128,128,128) },
    {  0,  0.0f, 3.2f,   0.1f, 1.3f, 3.3f, Color(128,128,128) },
    {  0,  0.0f, 3.6f,   0.1f, 1.3f, 3.7f, Color(128,128,128) },
};

SlabModel Posts = {sizeof(PostsSlabs)/sizeof(Slab), PostsSlabs};


// Temporary helper class used to initialize fills used by model. 
class FillCollection
{
public:
    Ptr<ShaderFill> LitSolid;
    Ptr<ShaderFill> LitTextures[Tex_Count];

	FillCollection(RenderDevice* render);
protected:
	void LoadTexture(const char* fname, Ptr<Texture> &builtinTexture, RenderDevice* render);
};

FillCollection::FillCollection(RenderDevice* render)
{
    Ptr<Texture> builtinTextures[Tex_Count];

    // Create floor checkerboard texture.
    {
        Color checker[256*256];
        for (int j = 0; j < 256; j++)
            for (int i = 0; i < 256; i++)
                checker[j*256+i] = (((i/4 >> 5) ^ (j/4 >> 5)) & 1) ?
                Color(180,180,180,255) : Color(80,80,80,255);
        builtinTextures[Tex_Checker] = *render->CreateTexture(Texture_RGBA|Texture_GenMipmaps, 256, 256, checker);
        builtinTextures[Tex_Checker]->SetSampleMode(Sample_Anisotropic|Sample_Repeat);
    }

    // Ceiling panel texture.
    {
        Color panel[256*256];
        for (int j = 0; j < 256; j++)
            for (int i = 0; i < 256; i++)
                panel[j*256+i] = (i/4 == 0 || j/4 == 0) ?
                Color(80,80,80,255) : Color(180,180,180,255);
        builtinTextures[Tex_Panel] = *render->CreateTexture(Texture_RGBA|Texture_GenMipmaps, 256, 256, panel);
        builtinTextures[Tex_Panel]->SetSampleMode(Sample_Anisotropic|Sample_Repeat);
    }

    // Wall brick textures.
    {
        Color block[256*256];
        for (int j = 0; j < 256; j++)
            for (int i = 0; i < 256; i++)
                block[j*256+i] = (((j/4 & 15) == 0) || (((i/4 & 15) == 0) && ((((i/4 & 31) == 0) ^ ((j/4 >> 4) & 1)) == 0))) ?
                Color(60,60,60,255) : Color(180,180,180,255);
        builtinTextures[Tex_Block] = *render->CreateTexture(Texture_RGBA|Texture_GenMipmaps, 256, 256, block);
        builtinTextures[Tex_Block]->SetSampleMode(Sample_Anisotropic|Sample_Repeat);
    }

	LoadTexture("images/field.png", builtinTextures[Tex_Table], render);
	for(int c = 0; c < sizeof(board.balls) / sizeof(*board.balls); c++){
		std::stringstream sbuf;
		if(c == 0)
			sbuf << "images/cueball.png";
		else
			sbuf << "images/ball" << c << ".png";
		LoadTexture(sbuf.rdbuf()->str().c_str(), builtinTextures[Tex_CueBall + c], render);
	}

	LitSolid = *new ShaderFill(*render->CreateShaderSet());
	LitSolid->GetShaders()->SetShader(render->LoadBuiltinShader(Shader_Vertex, VShader_MVP));
	LitSolid->GetShaders()->SetShader(render->LoadBuiltinShader(Shader_Fragment, FShader_LitGouraud));

	for (int i = 1; i < Tex_Count; i++)
	{
		LitTextures[i] = *new ShaderFill(*render->CreateShaderSet());
		LitTextures[i]->GetShaders()->SetShader(render->LoadBuiltinShader(Shader_Vertex, VShader_MVP));
		LitTextures[i]->GetShaders()->SetShader(render->LoadBuiltinShader(Shader_Fragment, FShader_LitTexture));
		LitTextures[i]->SetTexture(0, builtinTextures[i]);
	}

}

void FillCollection::LoadTexture(const char* fname, Ptr<Texture> &builtinTexture, RenderDevice* render){
	FILE *fp = fopen(fname, "rb");
	if(fp) try{
		unsigned char header[8];
		fread(header, 1, sizeof header, fp);
		bool is_png = !png_sig_cmp(header, 0, sizeof header);
		if(is_png){
			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, 
				[](png_structp, png_const_charp){}, [](png_structp, png_const_charp){});
			png_infop info_ptr = png_create_info_struct(png_ptr);
			if (!info_ptr)
			{
				png_destroy_read_struct(&png_ptr,
				(png_infopp)NULL, (png_infopp)NULL);
				throw 1;
			}
			png_infop end_info = png_create_info_struct(png_ptr);
			if (!end_info)
			{
				png_destroy_read_struct(&png_ptr, &info_ptr,
				(png_infopp)NULL);
				throw 2;
			}
			if (setjmp(png_jmpbuf(png_ptr))){
				png_destroy_read_struct(&png_ptr, &info_ptr,
				&end_info);
				fclose(fp);
				throw 3;
			}
			png_init_io(png_ptr, fp);
			png_set_sig_bytes(png_ptr, sizeof header);

			png_bytepp ret;

			{
				BITMAPINFO *bmi;
				png_uint_32 width, height;
				int bit_depth, color_type, interlace_type;
				int i;
				/* The native order of RGB components differs in order against Windows bitmaps,
				 * so we must instruct libpng to convert it. */
				png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, NULL);

				png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
					&interlace_type, NULL, NULL);

				/* Grayscale images are not supported.
				 * TODO: alpha channel? */
				if(bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA && color_type != PNG_COLOR_TYPE_PALETTE)
					throw 12;

				/* Calculate number of components. */
				int comps = (color_type == PNG_COLOR_TYPE_PALETTE ? 1 : color_type == PNG_COLOR_TYPE_RGB ? 3 : 4);

				// Supports paletted images
				png_colorp pal;
				int npal = 0;
				if(color_type == PNG_COLOR_TYPE_PALETTE){
					png_get_PLTE(png_ptr, info_ptr, &pal, &npal);
				}

				/* png_get_rows returns array of pointers to rows allocated by the library,
				 * which must be copied to single bitmap buffer. */
				ret = png_get_rows(png_ptr, info_ptr);


				std::vector<Color> panel;
				panel.resize(width * height);
				for (int j = 0; j < height; j++){
					for (int i = 0; i < width; i++){
						int idx = ret[j][i * comps];
						panel[j * width + i] = Color(pal[idx].red,pal[idx].green,pal[idx].blue,255);
					}
				}
				builtinTexture = *render->CreateTexture(Texture_RGBA|Texture_GenMipmaps, width, height, &panel.front());
				builtinTexture->SetSampleMode(Sample_Anisotropic|Sample_Repeat);
				
			}
		}
		fclose(fp);
	}
	catch(int e){
		fclose(fp);
	}
}

// Helper function to create a model out of Slab arrays.
Model* CreateModel(Vector3f pos, const SlabModel* sm, const FillCollection& fills)
{
    Model* m = new Model(Prim_Triangles);
    m->SetPosition(pos);

    for(int i=0; i< sm->Count; i++)
    {
        const Slab &s = sm->pSlabs[i];
        m->AddSolidColorBox(s.x1, s.y1, s.z1, s.x2, s.y2, s.z2, s.c);
    }

    if (sm->tex > 0)
        m->Fill = fills.LitTextures[sm->tex];
    else
        m->Fill = fills.LitSolid;
    return m;
}

Board board(-longEnd + inset, -shortEnd + inset, longEnd - inset, shortEnd - inset);

// Adds sample models and lights to the argument scene.
void PopulateRoomScene(Scene* scene, RenderDevice* render)
{
    FillCollection fills(render);  

    scene->World.Add(Ptr<Model>(*CreateModel(Vector3f(0,0,0),  &Room,       fills)));
    scene->World.Add(Ptr<Model>(*CreateModel(Vector3f(0,0,0),  &Floor,      fills)));
    scene->World.Add(Ptr<Model>(*CreateModel(Vector3f(0,0,0),  &Ceiling,    fills)));
    scene->World.Add(Ptr<Model>(*CreateModel(Vector3f(0,0,0),  &Table,  fills)));
	scene->World.Add(Ptr<Model>(*CreateModel(Vector3f(0,0,0),  &Cushions,  fills)));

	static const float ballRadius = Ball::defaultRadius;
	static const float ballDiameter = ballRadius * 2.f;

	board.init();

	Container *boardContainer = new Container;
	boardContainer->SetPosition(Vector3f(0, height + ballRadius, 0));

	board.init();

	for(int i = 0; i < sizeof(board.balls) / sizeof(*board.balls); i++){
		Ball &ball = board.balls[i];
		Model *model = new Model(Prim_Triangles);
		ball.model = model;
		model->AddSphere(ballRadius, Model::MirrorProjection, -1., -1., 1., 1.);
		model->Fill = fills.LitTextures[int(Tex_CueBall) + i];
		boardContainer->Add(Ptr<Model>(*model));
	}

	scene->World.Add(boardContainer);


    scene->SetAmbient(Vector4f(0.65f,0.65f,0.65f,1));
    scene->AddLight(Vector3f(-2,4,-2), Vector4f(8,8,8,1));
    scene->AddLight(Vector3f(3,4,-3),  Vector4f(2,1,1,1));
    scene->AddLight(Vector3f(-4,3,25), Vector4f(3,6,3,1));
}


// Render a debug marker static in rift (special request for eye-tracking)
void renderSphere(RenderDevice* render, Vector3f ViewAdjust, float metresLeft, float metresUp, float metresAway, float metresRadius,
				unsigned char red,unsigned char green,unsigned char blue)
{
	//Get textures, if haven't already
	static FillCollection * pfills;  
	static bool firstTime = true;
	if (firstTime)
	{
		firstTime=false;
		pfills = new FillCollection(render);
	}

	//Create object
	Scene*  scene = new Scene;
	Slab CubeSlabs[] =
	{
	#if 0 //Simple cube
		 { metresLeft-metresRadius,  metresUp-metresRadius, metresAway-metresRadius,
		   metresLeft+metresRadius,  metresUp+metresRadius, metresAway+metresRadius,  Color(red,green,blue) }
	#else //Blob
		 { metresLeft-0.33f*metresRadius,  metresUp-metresRadius, metresAway-0.33f*metresRadius,
		   metresLeft+0.33f*metresRadius,  metresUp+metresRadius, metresAway+0.33f*metresRadius,  Color(red,green,blue) },
		 { metresLeft-metresRadius,  metresUp-0.33f*metresRadius, metresAway-0.33f*metresRadius,
		   metresLeft+metresRadius,  metresUp+0.33f*metresRadius, metresAway+0.33f*metresRadius,  Color(red,green,blue) },
		 { metresLeft-0.33f*metresRadius,  metresUp-0.33f*metresRadius, metresAway-metresRadius,
		   metresLeft+0.33f*metresRadius,  metresUp+0.33f*metresRadius, metresAway+metresRadius,  Color(red,green,blue) },
		 { metresLeft-0.71f*metresRadius,  metresUp-0.71f*metresRadius, metresAway-0.71f*metresRadius,
		   metresLeft+0.71f*metresRadius,  metresUp+0.71f*metresRadius, metresAway+0.71f*metresRadius,  Color(red,green,blue) },

	#endif

	};
	SlabModel Cube = {sizeof(CubeSlabs)/sizeof(Slab), CubeSlabs, Tex_None};
    scene->World.Add(Ptr<Model>(*CreateModel(Vector3f(0,0,0),  &Cube,  *pfills)));
    scene->SetAmbient(Vector4f(1.0f,1.0f,1.0f,1));

	//Render object
    Matrix4f view = Matrix4f::LookAtRH(Vector3f(0,0,0), Vector3f(0,0,0) + Vector3f(0,0,1), Vector3f(0,1,0)); 
	scene->Render(render, Matrix4f::Translation(ViewAdjust) * view);

	//Delete object
	delete scene;
}

