/************************************************************************
 *									*
 *	Procedures for use bmp files					*
 *									*
 *	Author: Vincent LECUIRE, CRAN UMR 7039, Nancy-Universit , CNRS	*
 *	Date: march, 16 2012						*
 *									*
 ************************************************************************/

#include <stdint.h>

typedef struct BMP_struct {
	char	signature[2];	// BM (Bitmap windows), BA (Bitmap OS/2), CI (Icone couleur OS/2),...
	int	filesize;	// Taille totale du fichier BMP
	int	offset;
	int	headersize;
	int	imageHsize;
	int	imageVsize;
	int	plans;
	int	bpp;		// nombre de bits par pixel
	int	compression;
	int	imagesize;
	int	Hres;		// r solution horizontale
	int	Vres;		// r solution verticale
	int	colors;
	int	primarycolors;
	unsigned char *palette;
	double **data;
} BMPImageStruct;


  
int hex2dec (unsigned char *hex, int n) // Conversion d'un nombre cod  sur n octets (du poids fort au poids faible) en d cimal
{
	int value = 0;

	for (int i = n - 1; i >= 0; i--)	{  value = value * 256 + hex[i];  }
	return value;
}

void dec2hex (int value, unsigned char *hex, int n) // Conversion d'un entier en nombre cod  sur n octets (du poids fort au poids faible)
{	
	for (int i = 0; i < n; i++) hex[i]=0;

	for (int i = 0; i < n; i++)
	   {
  	     hex[i] =  value % 256;
  	     value = value / 256;
	   }
}

unsigned char max(unsigned char val1, unsigned char val2)
{
  if(val1 > val2)
    return val1;
  else
    return val2;
}

unsigned char min(unsigned char val1, unsigned char val2)
{
  if(val1 < val2)
    return val1;
  else
    return val2;
}


double **AllocateMemSpace(int Horizontal, int Vertical)
{
	double **Array;

	// Allocation m moire pour un tableau 2D de grande taille
	
	if ((Array = (double **) malloc(sizeof(double *) * Vertical))==NULL) return NULL;

	for (int i = 0; i < Vertical; i++)
		Array[i] = (double *) malloc(sizeof(double) * Horizontal);

	return Array;
}	



#if 0
int ReadBitmapFile_OLD (char* FileName,  BMPImageStruct *Image)
{
	unsigned char entier32[4]; //Zone m moire temporaire o  on encode les informations avant de les enregistrer
	unsigned char pixel;
	FILE * ImageFile;

	if ((ImageFile = fopen (FileName,"r")) == NULL) return -1;

	fread (Image->signature,2,1,ImageFile);		// signature cod e sur 2 octets
	fread (&entier32,sizeof (uint32_t),1,ImageFile);
	Image->filesize = hex2dec(entier32, sizeof(uint32_t));	// taille totale du fichier, 4 octets
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// reserv 
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// offset de d but de l'image, 4 octets
	Image->offset = hex2dec(entier32, sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// taille de l'entete, 4 octets
	Image->headersize = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// largeur de l'image, 4 octets
	Image->imageHsize = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// hauteur de l'image, 4 octets
	Image->imageVsize = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(short),1,ImageFile);		// nombre de plans (toujour =1), 2 octets
	Image->plans = hex2dec(entier32,2);
	fread (&entier32,sizeof(short),1,ImageFile);		// nombre de bits par pixel, 2 octets
	Image->bpp = hex2dec(entier32,sizeof(short));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// compression (0=rien), 4 octets
	Image->compression = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// taille de l'image, 4 octets
	Image->imagesize = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// r solution horizontale en pixels par m tre, 4 octets
	Image->Hres = hex2dec (entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// r solution verticale, en pixels par m tre, 4 octets
	Image->Vres = hex2dec (entier32,sizeof(4));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// nombre de couleurs utilis es (0=toutes), 4 octets
	Image->colors = hex2dec (entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// nombre de couleurs importantes (0=toutes), 4 octets
	Image->primarycolors = hex2dec (entier32,sizeof(uint32_t));
   
	// si c'est une image 8 bpp, cette ent te est suivie de la palette.
	if ((Image->palette = (unsigned char *) malloc(sizeof(unsigned char) * Image->colors * sizeof(uint32_t)))==NULL) return -1;
	for (int i = 0; i < Image->colors; i++) fread(&Image->palette[i*sizeof(uint32_t)],sizeof(uint32_t),1,ImageFile);

	// apr s la palette, ce sont les donn es de l'image
	if ((Image->data = AllocateMemSpace(Image->imageHsize, Image->imageVsize))==NULL) return -1;;

	for (int row=Image->imageVsize-1; row>=0; row--)
		for (int col=0;col<Image->imageHsize;col++)
			{
			    fread(&pixel,1,1,ImageFile);
				Image->data[row][col] = pixel;
			}
	
	fclose (ImageFile);
	return 0;
}
#endif

int ReadBitmapFile (char* FileName,  BMPImageStruct *Image)
{
	unsigned char entier32[4]; //Zone m�moire temporaire o� on encode les informations avant de les enregistrer
	unsigned char pixel;
	FILE * ImageFile;

	if ((ImageFile = fopen (FileName,"r")) == NULL) return -1;

	fread (Image->signature,2,1,ImageFile);		// signature cod�e sur 2 octets
	fread (&entier32,sizeof (uint32_t),1,ImageFile);
	Image->filesize = hex2dec(entier32, sizeof(uint32_t));	// taille totale du fichier, 4 octets
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// reserv�
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// offset de d�but de l'image, 4 octets
	Image->offset = hex2dec(entier32, sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// taille de l'entete, 4 octets
	Image->headersize = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// largeur de l'image, 4 octets
	Image->imageHsize = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// hauteur de l'image, 4 octets
	Image->imageVsize = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint16_t),1,ImageFile);		// nombre de plans (toujour =1), 2 octets
	Image->plans = hex2dec(entier32,2);
	fread (&entier32,sizeof(uint16_t),1,ImageFile);		// nombre de bits par pixel, 2 octets
	Image->bpp = hex2dec(entier32,sizeof(uint16_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// compression (0=rien), 4 octets
	Image->compression = hex2dec(entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// taille de l'image, 4 octets
	Image->imagesize = hex2dec(entier32,sizeof(long));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// r�solution horizontale en pixels par m�tre, 4 octets
	Image->Hres = hex2dec (entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// r�solution verticale, en pixels par m�tre, 4 octets
	Image->Vres = hex2dec (entier32,sizeof(long));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// nombre de couleurs utilis�es (0=toutes), 4 octets
	Image->colors = hex2dec (entier32,sizeof(uint32_t));
	fread (&entier32,sizeof(uint32_t),1,ImageFile);		// nombre de couleurs importantes (0=toutes), 4 octets
	Image->primarycolors = hex2dec (entier32,sizeof(uint32_t));

	// si c'est une image 8 bpp, cette ent�te est suivie de la palette.
	if ((Image->palette = (unsigned char *) malloc(sizeof(unsigned char) * Image->colors * sizeof(uint32_t)))==NULL) return -1;
	for (int i = 0; i < Image->colors; i++) fread(&Image->palette[i*sizeof(uint32_t)],sizeof(uint32_t),1,ImageFile);

	// apr�s la palette, ce sont les donn�es de l'image

	if ((Image->data = AllocateMemSpace(Image->imageHsize, Image->imageVsize))==NULL) return -1;;

	for (int row=Image->imageVsize-1; row>=0; row--)
		for (int col=0;col<Image->imageHsize;col++)
			{
			    fread(&pixel,1,1,ImageFile);
				Image->data[row][col] = pixel;
			}
	
	fclose (ImageFile);
	return 0;
}

int WriteBitmapFile (char* FileName,  BMPImageStruct *Image)
{
	unsigned char entier32[4]; //Zone m moire temporaire o  on encode les informations avant de les enregistrer
	unsigned char pixel;
	FILE * ImageFile;

	if ((ImageFile = fopen (FileName,"w")) == NULL) return -1;

	fwrite (Image->signature,2,1,ImageFile);		// signature cod e sur 2 octets
	dec2hex(Image->filesize, entier32, sizeof(4));	// taille totale du fichier, 4 octets
	fwrite (&entier32,sizeof (4),1,ImageFile);
	dec2hex(0, entier32, sizeof(4));
	fwrite (&entier32,sizeof (4),1,ImageFile);
	dec2hex(Image->offset, entier32, sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// offset de d but de l'image, 4 octets
	dec2hex(Image->headersize,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// taille de l'entete, 4 octets
	dec2hex(Image->imageHsize,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// largeur de l'image, 4 octets
	dec2hex(Image->imageVsize,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// hauteur de l'image, 4 octets
	dec2hex(Image->plans,entier32,2);
	fwrite (&entier32,sizeof(uint16_t),1,ImageFile);		// nombre de plans (toujour =1), 2 octets
	dec2hex(Image->bpp,entier32,sizeof(uint16_t));
	fwrite (&entier32,sizeof(uint16_t),1,ImageFile);		// nombre de bits par pixel, 2 octets
	dec2hex(Image->compression,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// compression (0=rien), 4 octets
	dec2hex(Image->imagesize,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// taille de l'image, 4 octets
	dec2hex (Image->Hres,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// r solution horizontale en pixels par m tre, 4 octets
	dec2hex (Image->Vres,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// r solution verticale, en pixels par m tre, 4 octets
	dec2hex (Image->colors,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// nombre de couleurs utilis es (0=toutes), 4 octet 
	dec2hex (Image->primarycolors,entier32,sizeof(4));
	fwrite (&entier32,sizeof(4),1,ImageFile);		// nombre de couleurs importantes (0=toutes), 4 octets
		
	// si c'est une image 8 bpp, cette ent te est suivie de la palette.
	for (int i = 0; i < Image->colors; i++) fwrite(&Image->palette[i*sizeof(uint32_t)],sizeof(uint32_t),1,ImageFile);

	// apr s la palette, ce sont les donn es de l'image
	for (int row=Image->imageVsize-1; row>=0; row--)
		for (int col=0;col<Image->imageHsize;col++)
			{
			    pixel = (unsigned char) Image->data[row][col];
			    fwrite(&pixel,1,1,ImageFile);
			}

	fclose(ImageFile);
	return 0;
}
