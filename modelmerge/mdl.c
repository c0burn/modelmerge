#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef float vec3_t[3];

struct mdl_header_t
{
	int ident;		// magic number: "IDPO" / 1330660425
	int version;	// version: 6 

	vec3_t scale;
	vec3_t translate;
	float boundingradius;
	vec3_t eyeposition;

	int num_skins;
	int skinwidth;
	int skinheight;

	int num_verts;
	int num_tris;
	int num_frames;

	int synctype;
	int flags;
	float size;
};

struct mdl_skin_t
{
	int group;
	unsigned char *data;
};

struct mdl_texcoord_t
{
	int onseam;
	int s;
	int t;
};

struct mdl_triangle_t
{
	int facesfront;
	int vertex[3];
};

struct mdl_vertex_t
{
	unsigned char v[3];
	unsigned char normalIndex;
};

struct mdl_simpleframe_t
{
	struct mdl_vertex_t bboxmin;
	struct mdl_vertex_t bboxmax;
	char name[16];
	struct mdl_vertex_t *verts;
};

struct mdl_frame_t
{
	int type;
	struct mdl_simpleframe_t frame;
};

struct mdl_model_t
{
	struct mdl_header_t header;
	struct mdl_skin_t *skins;
	struct mdl_texcoord_t *texcoords;
	struct mdl_triangle_t *triangles;
	struct mdl_frame_t *frames;
};

struct mdl_model_t mdlfile1;
struct mdl_model_t mdlfile2;

/*==========
ReadMDL

reads a model from disk and puts it in a mdl_model_t struct
==========*/
int ReadMDL (const char *filename, struct mdl_model_t *mdl)
{
	printf("reading model: %s\n", filename);
	FILE *fp = fopen(filename, "rb");
	if (!fp)
	{
		fprintf(stderr, "error: couldn't open \"%s\"!\n", filename);
		return 0;
	}

	// read header
	fread(&mdl->header, 1, sizeof(struct mdl_header_t), fp);
	if ((mdl->header.ident != 1330660425) || (mdl->header.version != 6))
	{
		fprintf(stderr, "error: bad version or identifier\n");
		fclose(fp);
		return 0;
	}
	printf("verts: %u triangles: %u frames: %u\n", mdl->header.num_verts, mdl->header.num_tris, mdl->header.num_frames);
	printf("skins: %u width: %u height: %u\n", mdl->header.num_skins, mdl->header.skinwidth, mdl->header.skinheight);
	
	// allocate memory for skins, vertices, triangles and frames
	mdl->skins = malloc(sizeof(struct mdl_skin_t) * mdl->header.num_skins);
	mdl->texcoords = malloc(sizeof(struct mdl_texcoord_t) * mdl->header.num_verts);
	mdl->triangles = malloc(sizeof(struct mdl_triangle_t) * mdl->header.num_tris);
	mdl->frames = malloc(sizeof(struct mdl_frame_t) * mdl->header.num_frames);
  
	// read skins
	for (int i = 0; i < mdl->header.num_skins; i++)
	{
		printf("reading skin: %u\n", i);
		mdl->skins[i].data = malloc(sizeof(unsigned char) * mdl->header.skinwidth * mdl->header.skinheight);
		fread(&mdl->skins[i].group, sizeof(int), 1, fp);
		fread(mdl->skins[i].data, sizeof(unsigned char), mdl->header.skinwidth * mdl->header.skinheight, fp);
	}

	// read vertices tris and frames
	printf("reading vertices, tris and frames\n");
	fread(mdl->texcoords, sizeof(struct mdl_texcoord_t), mdl->header.num_verts, fp);
	fread(mdl->triangles, sizeof(struct mdl_triangle_t), mdl->header.num_tris, fp);
	for (int i = 0; i < mdl->header.num_frames; ++i)
	{
		mdl->frames[i].frame.verts = malloc(sizeof(struct mdl_vertex_t) * mdl->header.num_verts);
		fread(&mdl->frames[i].type, sizeof(int), 1, fp);
		fread(&mdl->frames[i].frame.bboxmin, sizeof(struct mdl_vertex_t), 1, fp);
		fread(&mdl->frames[i].frame.bboxmax, sizeof(struct mdl_vertex_t), 1, fp);
		fread(mdl->frames[i].frame.name, sizeof(char), 16, fp);
		fread(mdl->frames[i].frame.verts, sizeof(struct mdl_vertex_t), mdl->header.num_verts, fp);
	}

	printf("success!\n\n");
	fclose(fp);
	return 1;
}

/*==========
MDLCheckBackfaces

returns 1 if the model contains backfacing triangles that contain onseam vertices

we need to know this because these vertices are moved by += skinwidth / 2 by the engine when rendering the model
==========*/
int MDLCheckBackfaces(struct mdl_model_t *mdl)
{
	for (int i = 0; i < mdl->header.num_tris; i++)
	{
		if (!(mdl->triangles[i].facesfront))
		{
			for (int j = 0; j < 3; j++)
			{
				if (mdl->texcoords[mdl->triangles[i].vertex[j]].onseam)
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

/*==========
MergeMDL
==========*/
int MergeMDL(const char *filename, struct mdl_model_t *mdl)
{
	printf("saving new model \"%s\"\n", filename);
	FILE *fp = fopen(filename, "wb");
	if (!fp)
	{
		fprintf(stderr, "error: couldn't open \"%s\"!\n", filename);
		return 0;
	}

	// store off some information we'll need later
	int width1 = mdlfile1.header.skinwidth;
	int width2 = mdlfile2.header.skinwidth;
	int height1 = mdlfile1.header.skinheight;
	int height2 = mdlfile2.header.skinheight;
	int verts1 = mdlfile1.header.num_verts;
	int verts2 = mdlfile2.header.num_verts;
	int tris1 = mdlfile1.header.num_tris;
	int tris2 = mdlfile2.header.num_tris;
	int skins1 = mdlfile1.header.num_skins;
	int skins2 = mdlfile2.header.num_skins;

	printf("model 1: ");
	int backfaces1 = MDLCheckBackfaces(&mdlfile1);
	printf("model 2: ");
	int backfaces2 = MDLCheckBackfaces(&mdlfile2);

	// write new header
	// use the mdlfile1 header as a base, and adjust as needed
	mdl->header.skinwidth = width1 + width2;
	if (!backfaces1 && backfaces2)
		mdl->header.skinwidth += width1;
	if (!backfaces2 && backfaces1)
		mdl->header.skinwidth += width2;
	if (height1 < height2)
		mdl->header.skinheight = height2;
	else
		mdl->header.skinheight = height1;
	mdl->header.num_verts = verts1 + verts2;
	mdl->header.num_tris = tris1 + tris2;
	if (skins1 < skins2)
		mdl->header.num_skins = skins2;
	else
		mdl->header.num_skins = skins1;
	printf("verts: %u triangles: %u frames: %u\n", mdl->header.num_verts, mdl->header.num_tris, mdl->header.num_frames);
	printf("skins: %u width: %u height: %u\n", mdl->header.num_skins, mdl->header.skinwidth, mdl->header.skinheight);
	printf("writing new header\n");
	fwrite(&mdl->header, 1, sizeof(struct mdl_header_t), fp);
	
	// calculate new skins
	for (int i = 0; i < mdl->header.num_skins; i++)
	{
		printf("writing skin: %u\n", i);

		// if models have uneven skin numbers then write skin 0 as a default
		int skin1 = i;
		int skin2 = i;
		if (i > skins1 - 1)
			skin1 = 0;
		if (i > skins2 - 1)
			skin2 = 0;

		fwrite(&mdl->skins[skin1].group, sizeof(int), 1, fp);

		for (int j = 0; j < mdl->header.skinheight; j++)
		{
			if (backfaces1)
			{
				for (int k = 0; k < width1 / 2; k++)
				{
					if (j < height1)
						fputc(mdlfile1.skins[skin1].data[j * width1 + k], fp);
					else
						fputc(255, fp);
				}
			}
			else
			{
				for (int k = 0; k < width1; k++)
				{
					if (j < height1)
						fputc(mdlfile1.skins[skin1].data[j * width1 + k], fp);
					else
						fputc(255, fp);
				}
			}
			
			if (backfaces2)
			{
				for (int k = 0; k < width2 / 2; k++)
				{
					if (j < height2)
						fputc(mdlfile2.skins[skin2].data[j * width2 + k], fp);
					else
						fputc(255, fp);
				}
			}
			else
			{
				for (int k = 0; k < width2; k++)
				{
					if (j < height2)
						fputc(mdlfile2.skins[skin2].data[j * width2 + k], fp);
					else
						fputc(255, fp);
				}
			}

			// we can skip anything else if both models are entirely composed of front-facing triangles
			if (!backfaces1 && !backfaces2)
				continue;

			if (backfaces1)
			{
				for (int k = width1 / 2; k < width1; k++)
				{
					if (j < height1)
						fputc(mdlfile1.skins[skin1].data[j * width1 + k], fp);
					else
						fputc(255, fp);
				}
			}
			else
			{
				for (int k = 0; k < width1; k++)
					fputc(255, fp);
			}

			if (backfaces2)
			{
				for (int k = width2 / 2; k < width2; k++)
				{
					if (j < height2)
						fputc(mdlfile2.skins[skin2].data[j * width2 + k], fp);
					else
						fputc(255, fp);
				}
			}
			else
			{
				for (int k = 0; k < width2; k++)
					fputc(255, fp);
			}
		}
	}

	// fix vertices (texture coordinates)
	printf("adjusting vertices (texture coordinates) in model 1\n");
	for (int i = 0; i < verts1; i++)
	{
		if (backfaces1)
		{
			if (mdlfile1.texcoords[i].s > width1 / 2)
			{
				if (backfaces2)
					mdlfile1.texcoords[i].s += width2 / 2;
				else
					mdlfile1.texcoords[i].s += width2;
			}
		}
	}
	printf("adjusting vertices (texture coordinates) in model 2\n");
	for (int i = 0; i < verts2; i++)
	{
		if (backfaces2)
		{
			if (mdlfile2.texcoords[i].s > width2 / 2)
			{
				if (backfaces1)
					mdlfile2.texcoords[i].s += width1 / 2;
				else
					mdlfile2.texcoords[i].s += width1;
			}
		}
		if (backfaces1)
			mdlfile2.texcoords[i].s += width1 / 2;
		else
			mdlfile2.texcoords[i].s += width1;
	}
	printf("writing new vertices\n"); 
	fwrite(mdlfile1.texcoords, sizeof(struct mdl_texcoord_t), verts1, fp);
	fwrite(mdlfile2.texcoords, sizeof(struct mdl_texcoord_t), verts2, fp);

	// fix triangles
	for (int i = 0; i < tris2; i++)
	{
		// triangle vertexes are indices, so we just need to increase them by verts1
		mdlfile2.triangles[i].vertex[0] += verts1;
		mdlfile2.triangles[i].vertex[1] += verts1;
		mdlfile2.triangles[i].vertex[2] += verts1;
	}
	printf("writing new triangles\n");
	fwrite(mdlfile1.triangles, sizeof(struct mdl_triangle_t), tris1, fp);
	fwrite(mdlfile2.triangles, sizeof(struct mdl_triangle_t), tris2, fp);

	// fix frames
	// first we transform model2's vertices into model1's coordinate space, and store off the biggest value for later use
	printf("fixing frames\n");
	float maxpos = 0;
	float *newverts = calloc(verts2 * 3, sizeof(float));
	for (int i = 0; i < verts2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			newverts[i*3 + j] = mdlfile2.frames[0].frame.verts[i].v[j] * mdlfile2.header.scale[j] + mdlfile2.header.translate[j];
			newverts[i*3 + j] = (newverts[i*3 + j] - mdlfile1.header.translate[j]) / mdlfile1.header.scale[j];
			if (newverts[i*3 + j] > maxpos)
				maxpos = newverts[i*3 + j];
		}
	}
	// then we convert the new vertices back into unsigned chars, scaling them 0-255 if needed
	for (int i = 0; i < verts2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			// we only use the 1st frame from model2
			mdlfile2.frames[0].frame.verts[i].v[j] = (unsigned char)((newverts[i*3 + j] / maxpos) * 255);
		}
	}
	free(newverts);

	printf("writing new frames\n");
	for (int i = 0; i < mdl->header.num_frames; ++i)
	{
		fwrite(&mdl->frames[i].type, sizeof(int), 1, fp);
		fwrite(&mdl->frames[i].frame.bboxmin, sizeof(struct mdl_vertex_t), 1, fp);
		fwrite(&mdl->frames[i].frame.bboxmax, sizeof(struct mdl_vertex_t), 1, fp);
		fwrite(mdl->frames[i].frame.name, sizeof(char), 16, fp);
		fwrite(mdl->frames[i].frame.verts, sizeof(struct mdl_vertex_t), verts1, fp);
		// we always use the 1st frame from model2
		fwrite(mdlfile2.frames[0].frame.verts, sizeof(struct mdl_vertex_t), verts2, fp);
	}

	printf("success!\n\n");
	fclose(fp);
	return 1;
}

/*==========
FreeMDL
==========*/
void FreeMDL(struct mdl_model_t *mdl)
{
	if (mdl->skins)
	{
		free(mdl->skins);
		mdl->skins = NULL;
	}

	if (mdl->texcoords)
	{
		free(mdl->texcoords);
		mdl->texcoords = NULL;
	}

	if (mdl->triangles)
	{
		free(mdl->triangles);
		mdl->triangles = NULL;
	}

	if (mdl->frames)
	{
		for (int i = 0; i < mdl->header.num_frames; ++i)
		{
			free(mdl->frames[i].frame.verts);
			mdl->frames[i].frame.verts = NULL;
		}
		free(mdl->frames);
		mdl->frames = NULL;
	}
}

/*==========
cleanup
==========*/
void cleanup (void)
{
	FreeMDL(&mdlfile1);
	FreeMDL(&mdlfile2);
}

/*==========
main
==========*/
int main(int argc, char *argv[])
{
	printf("modelmerge - compiled on %s %s\n\n", __DATE__, __TIME__);

	if (argc > 1)
	{
		if (strchr(argv[1], '?') != NULL)
		{
			fprintf(stderr, "usage: <input1.mdl> <input2.mdl> <output.mdl> (optional)\n");
			fprintf(stderr, "the 3rd argument is optional and defaults to \"output.mdl\"\n");
			exit(EXIT_SUCCESS);
		}
	}

	if (argc < 3 || argc > 4)
	{
		fprintf(stderr, "wrong arguments!\n");
		fprintf(stderr, "usage: <input1.mdl> <input2.mdl> <output.mdl> (optional)\n");
		fprintf(stderr, "the 3rd argument is defaults to \"output.mdl\" in the current directory\n");
		exit(EXIT_FAILURE);
	}
	
	atexit(cleanup);

	if (!ReadMDL(argv[1], &mdlfile1))
		exit(EXIT_FAILURE);
	if (!ReadMDL(argv[2], &mdlfile2))
		exit(EXIT_FAILURE);
	
	/*
	if (!ReadMDL("player.mdl", &mdlfile1))
		exit(EXIT_FAILURE);
	if (!ReadMDL("gun.mdl", &mdlfile2))
		exit(EXIT_FAILURE);
	*/

	if (argc > 3)
		MergeMDL(argv[3], &mdlfile1);
	else
		MergeMDL("output.mdl", &mdlfile1);

	return 0;
}
