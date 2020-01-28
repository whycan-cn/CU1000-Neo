#include "compiler.h"

#define LCD_BPP 5
#define LOGO_FILES 6

enum {
	MODE_GEN_INFO,
	MODE_GEN_DATA
};

typedef struct bitmap_s {		/* bitmap description */
	uint16_t width;
	uint16_t height;
	uint8_t	palette[256*3];
	uint8_t	*data;
} bitmap_t;

#define DEFAULT_CMAP_SIZE	16	/* size of default color map	*/

void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [--gen-info|--gen-data] file\n", prog);
}

/*
 * Neutralize little endians.
 */
uint16_t le_short(uint16_t x)
{
    uint16_t val;
    uint8_t *p = (uint8_t *)(&x);

    val =  (*p++ & 0xff) << 0;
    val |= (*p & 0xff) << 8;

    return val;
}

void skip_bytes (FILE *fp, int n)
{
	while (n-- > 0)
		fgetc (fp);
}

__attribute__ ((__noreturn__))
int error (char * msg, FILE *fp)
{
	fprintf (stderr, "ERROR: %s\n", msg);

	fclose (fp);

	exit (EXIT_FAILURE);
}

void gen_info_char(bitmap_t *b, uint16_t n_colors, int nbit)
{
	printf("/*\n"
		" * Automatically generated by \"tools/bmp_logo\"\n"
		" *\n"
		" * DO NOT EDIT\n"
		" *\n"
		" */\n\n\n"
		"#ifndef __BMP_LOGO_H__\n"
		"#define __BMP_LOGO_H__\n\n"
		"#define BMP_LOGO_WIDTH\t\t%d\n"
		"#define BMP_LOGO_HEIGHT\t\t%d\n"
		"#define BMP_LOGO_COLORS\t\t%d\n"
		"#define BMP_LOGO_OFFSET\t\t%d\n\n"
		"#define BMP_LOGO_NBIT\t\t%d\n\n"
		"extern unsigned short bmp_logo_palette[];\n"
		"extern unsigned char bmp_logo_bitmap[];\n\n"
		"#endif /* __BMP_LOGO_H__ */\n",
		b->width, b->height, n_colors,
		DEFAULT_CMAP_SIZE, nbit);
}

void gen_info_int(bitmap_t *b, uint16_t n_colors, int nbit)
{
	printf("/*\n"
		" * Automatically generated by \"tools/bmp_logo\"\n"
		" *\n"
		" * DO NOT EDIT\n"
		" *\n"
		" */\n\n\n"
		"#ifndef __BMP_LOGO_H__\n"
		"#define __BMP_LOGO_H__\n\n"
		"#define BMP_LOGO_WIDTH\t\t%d\n"
		"#define BMP_LOGO_HEIGHT\t\t%d\n"
		"#define BMP_LOGO_COLORS\t\t%d\n"
		"#define BMP_LOGO_OFFSET\t\t%d\n\n"
		"#define BMP_LOGO_NBIT\t\t%d\n\n"
		"extern unsigned short bmp_logo_palette[];\n"
		"extern unsigned int bmp_logo_bitmap[];\n\n"
		"#endif /* __BMP_LOGO_H__ */\n",
		b->width, b->height, n_colors,
		DEFAULT_CMAP_SIZE, nbit);
}


int main (int argc, char *argv[])
{
	int	mode = -1, i, j, x;
	FILE	*fp;
	uint16_t data_offset, n_colors = 0;
	int nbit = 0;

	if (argc < 3) {
		usage(argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!strcmp(argv[1], "--gen-info"))
		mode = MODE_GEN_INFO;
	else if (!strcmp(argv[1], "--gen-data"))
		mode = MODE_GEN_DATA;
	if(mode == MODE_GEN_INFO || mode == MODE_GEN_DATA) {
		bitmap_t bmp;
		bitmap_t *b = &bmp;
		fp = fopen(argv[2], "rb");
		if (!fp) {
			perror(argv[2]);
			exit (EXIT_FAILURE);
		}
		if (fgetc (fp) != 'B' || fgetc (fp) != 'M')
			error ("Input file is not a bitmap", fp);
		/*
		 * read width and height of the image, and the number of colors used;
		 * ignore the rest
		 */
		skip_bytes (fp, 8);
		if (fread (&data_offset, sizeof (uint16_t), 1, fp) != 1)
			error ("Couldn't read bitmap data offset", fp);
		skip_bytes (fp, 6);
		if (fread (&b->width,   sizeof (uint16_t), 1, fp) != 1)
			error ("Couldn't read bitmap width", fp);
		skip_bytes (fp, 2);
		if (fread (&b->height,  sizeof (uint16_t), 1, fp) != 1)
			error ("Couldn't read bitmap height", fp);
		skip_bytes(fp,4);
		if (fread (&nbit,  sizeof (uint16_t), 1, fp) != 1)
			error ("Couldn't read bitmap nbit", fp);
		if(nbit == 24)
			skip_bytes (fp, 24);
		else{
			skip_bytes (fp, 16);
			if(fread (&n_colors, sizeof (uint16_t), 1, fp) != 1)
				error ("Couldn't read bitmap colors", fp);
			skip_bytes (fp, 6);
		}

		/*
		 * Repair endianess.
		 */
		data_offset = le_short(data_offset);
		b->width = le_short(b->width);
		b->height = le_short(b->height);
		n_colors = le_short(n_colors);

		if(nbit != 24) {
			/* assume we are working with an 8-bit file */
			if ((n_colors == 0) || (n_colors > 256 - DEFAULT_CMAP_SIZE)) {
				/* reserve DEFAULT_CMAP_SIZE color map entries for default map */
				n_colors = 256 - DEFAULT_CMAP_SIZE;
			}
		}

		if (mode == MODE_GEN_INFO) {
			if(nbit == 24)
				gen_info_int(b, n_colors, nbit);
			else
				gen_info_char(b, n_colors, nbit);
			fclose(fp);
			return 0;
		}

		printf("/*\n"
				" * Automatically generated by \"tools/bmp_logo\"\n"
				" *\n"
				" * DO NOT EDIT\n"
				" *\n"
				" */\n\n\n"
				"#ifndef __BMP_LOGO_DATA_H__\n"
				"#define __BMP_LOGO_DATA_H__\n\n");

		/* allocate memory */
		if ((b->data = (uint8_t *)malloc(b->width * b->height * nbit / 8)) == NULL)
			error ("Error allocating memory for file", fp);
		/* 8bit*/
		if(nbit != 24) {
			/* read and print the palette information */
			printf("unsigned short bmp_logo_palette[] = {\n");

			for (i=0; i<n_colors; ++i) {
				b->palette[(int)(i*3+2)] = fgetc(fp);
				b->palette[(int)(i*3+1)] = fgetc(fp);
				b->palette[(int)(i*3+0)] = fgetc(fp);
				x=fgetc(fp);

				printf ("%s0x0%X%X%X,%s",
						((i%8) == 0) ? "\t" : "  ",
						(b->palette[(int)(i*3+0)] >> 4) & 0x0F,
						(b->palette[(int)(i*3+1)] >> 4) & 0x0F,
						(b->palette[(int)(i*3+2)] >> 4) & 0x0F,
						((i%8) == 7) ? "\n" : ""
				       );
			}

			/* seek to offset indicated by file header */
			fseek(fp, (long)data_offset, SEEK_SET);

			/* read the bitmap; leave room for default color map */
			printf ("\n");
			printf ("};\n");
			printf ("\n");
			printf("unsigned char bmp_logo_bitmap[] = {\n");
			for (i=(b->height-1)*b->width; i>=0; i-=b->width) {
				for (x = 0; x < b->width; x++) {
					b->data[(uint16_t) i + x] = (uint8_t) fgetc (fp) \
								    + DEFAULT_CMAP_SIZE;
				}
				if (b->width % 4)
					skip_bytes (fp, 4 - b->width % 4);
			}

			for (i=0; i<(b->height*b->width); ++i) {
				if ((i%8) == 0)
					putchar ('\t');
				printf ("0x%02X,%c",
						b->data[i],
						((i%8) == 7) ? '\n' : ' '
				       );
			}
			printf ("\n"
					"};\n\n"
					"#endif /* __BMP_LOGO_DATA_H__ */\n"
			       );
		}
		if (nbit == 24) {
			uint8_t tmp1,tmp2,tmp3,tmp4;
			int value;
			int bitPerLine,i,j, k;
			int padding = 0;
			unsigned char data;

			printf ("unsigned short bmp_logo_palette[] = {\n");
			printf ("\n");
			printf ("};\n");
			printf ("\n");

			printf ("unsigned int bmp_logo_bitmap[] = {\n");
			printf ("\n\n");

			fseek(fp, (long)data_offset, SEEK_SET);

			tmp4 = b->width % 4;

			for (i=0; i < b->height; i++) {
				for (j=0; j< b->width; j++) {
					fread(&b->data,sizeof(uint8_t),1,fp);
					tmp1=b->data;
					fread(&b->data,sizeof(uint8_t),1,fp);
					tmp2=b->data;
					fread(&b->data,sizeof(uint8_t),1,fp);
					tmp3=b->data;
					value=(tmp1 & 0xff) << 0 | (tmp2 & 0xff )<< 8 | ( tmp3 & 0xff )<< 16 | 0x00 << 24;
					printf (" 0x%08X, %s", value, ((j%8) == 7) ? "\n" : "");
				}
				fread(&b->data,sizeof(uint8_t),tmp4,fp);
			}
			printf ("\n};\n");
			printf ("#endif /* __BMP_LOGO_H__ */\n");
		}
		fclose(fp);
		return 0;
	}
	else {
		bitmap_t bmp[LOGO_FILES];
		bitmap_t *b = &bmp[0];

#if LCD_BPP==4
		short val;
#elif LCD_BPP==5
		int val;
#endif
		if (argc < (LOGO_FILES + 1)) {
			fprintf (stderr, "Usage: %d bmp files need\n", LOGO_FILES);
			exit (EXIT_FAILURE);
		}
		j = 0;
		for (j = 0; j < LOGO_FILES; j ++) {
			if ((fp = fopen (argv[j+1], "rb")) == NULL) {
				perror (argv[j+1]);
				exit (EXIT_FAILURE);
			}

			if (fgetc (fp) != 'B' || fgetc (fp) != 'M') {
				fprintf (stderr, "%s is not a bitmap file.\n", argv[j+1]);
				exit (EXIT_FAILURE);
			}
			fclose(fp);
		}

		/*
		 * read width and height of the image, and the number of colors used;
		 * ignore the rest
		 */
		fp = fopen (argv[1], "rb");
		skip_bytes (fp, 10); //skip 8 byte
		fread (&data_offset, sizeof (uint16_t), 1, fp); //read size is 2
		skip_bytes (fp, 6);
		fread (&b->width,   sizeof (uint16_t), 1, fp); //read size is 2
		skip_bytes (fp, 2);
		fread (&b->height,  sizeof (uint16_t), 1, fp);

		skip_bytes(fp,4);
		fread (&nbit,  sizeof (uint16_t), 1, fp);

		if(nbit == 24)
			skip_bytes (fp, 24);
		else{
			skip_bytes (fp, 16);
			fread (&n_colors, sizeof (uint16_t), 1, fp);
			skip_bytes (fp, 6);
		}
		/*
		 * Repair endianess.
		 */
		data_offset = le_short(data_offset);
		b->width = le_short(b->width);
		b->height = le_short(b->height);
		n_colors = le_short(n_colors);

		/* whether file is 24bit or not */
		if(nbit == 24){
			if ( n_colors > 16777216 ) {
				n_colors = 16777216 ;
			}
		}
		else {
			/* file is 8 bit*/
			if ( n_colors > 256 ) {
				n_colors = 256 ;
			}
		}

		printf ("/*\n"
				" * Automatically generated by \"tools/bmp_logo\"\n"
				" *\n"
				" * DO NOT EDIT\n"
				" *\n"
				" */\n\n\n"
				"#ifndef __CHARGE_LOGO_H__\n"
				"#define __CHARGE_LOGO_H__\n\n"
				"#define CHARGE_LOGO_WIDTH\t\t%d\n"
				"#define CHARGE_LOGO_HEIGHT\t\t%d\n"
				"#define CHARGE_LOGO_COLORS\t\t%ld\n"
				"#define CHARGE_LOGO_OFFSET\t\t%d\n"
				"#define CHARGE_LOGO_BITS\t\t%ld\n"
				,
				b->width, b->height, n_colors,
				DEFAULT_CMAP_SIZE,nbit);

		/* allocate memory */
		if ((b->data = (uint8_t *)malloc(b->width * b->height * nbit/8)) == NULL) {
			fclose (fp);
			printf ("Error allocating memory for file %s.\n", argv[1]);
			exit (EXIT_FAILURE);
		}

		//----------------------------read 8-bit bmp----------------------------------------------//
		if( nbit == 8){
			/* read and print the palette information */
			printf ("unsigned int charge_logo_palette[] = {\n");

			for (i=0; i<n_colors; ++i) {
				b->palette[(int)(i*3+2)] = fgetc(fp);
				b->palette[(int)(i*3+1)] = fgetc(fp);
				b->palette[(int)(i*3+0)] = fgetc(fp);
				x=fgetc(fp);
				val = 0;
#if LCD_BPP==4
				/* RGB565(16bits) */
				val = 	((b->palette[(int)(i*3+0)] >> 3) & 0x1F )<< 11 | \
					((b->palette[(int)(i*3+1)] >> 2) & 0x3F )<< 5  | \
					((b->palette[(int)(i*3+2)] >> 3) & 0x1F );
#elif LCD_BPP==5
				/* RGB666(18bits) */
				val = 	b->palette[(int)(i*3+0)] << 16 |	\
					b->palette[(int)(i*3+1)] << 8  |	\
					b->palette[(int)(i*3+2)] << 0 ;
#endif
				printf ("%s0x%X,%s",
						((i%8) == 0) ? "\t" : "  ",
						val,
						((i%8) == 7) ? "\n" : ""
				       );
			}

			/* seek to offset indicated by file header */
			fseek(fp, (long)data_offset, SEEK_SET);

			/* read the bitmap; leave room for default color map */

			printf ("\n");
			printf ("};\n");
			printf ("\n");
			printf ("unsigned char charge_logo_bitmap[%d][%d] = {\n", LOGO_FILES, b->height*b->width);
			for (i=(b->height-1)*b->width; i>=0; i-=b->width) {
				for (x = 0; x < b->width; x++) {
					b->data[ i + x] = (uint8_t) fgetc (fp) ;
				}
			}
			fclose (fp);

			for (i=0; i<(b->height*b->width); ++i) {
				if ((i%8) == 0)
					putchar ('\t');
				printf ("0x%02X,%c",
						b->data[i],
						((i%8) == 7) ? '\n' : ' '
				       );
			}
			printf ("\n"
					"};\n\n"
					"#endif /* __BMP_LOGO_H__ */\n"
			       );
		} /* nbit == 8 end*/
		//----------------------------read 24-bit bmp----------------------------------------------//
		else {
			/*get rgb of 24-bit file*/
			uint8_t tmp1,tmp2,tmp3,tmp4;
			int value;
			int bitPerLine,i,j, k;
			int padding = 0;
			unsigned char data;

			bitPerLine  = ( (b->width*3)%4==0 ) ? b->width*3 : b->width*3 + (4- (b->width*3)%4) ;
			/*padding width */
			padding = ( (b->width*3)%4==0 )? 0 : (4 - ((b->width * 3)) % 4);

			printf(" // bitPerLine=%d \n",bitPerLine);

			printf ("unsigned int charge_logo_palette[] = {\n");

			printf ("\n");
			printf ("};\n");

			printf ("\n");

			/* seek to offset indicated by file header */
			//		fseek(fp, (long)data_offset, SEEK_SET);
			printf ("unsigned int charge_logo_bitmap[%d][%d] = {\n", LOGO_FILES, b->height*b->width);
			for (k = 0; k < LOGO_FILES; k++) {
				fp = fopen(argv[k+1], "rb");
				printf("{\n");
				fseek(fp, (long)data_offset, SEEK_SET);

				for (i=0; i < b->height; i++) {
					printf ("\n\n");

					for (j=0; j< b->width; j++) {

						fread(&b->data,sizeof(uint8_t),1,fp);
						tmp1=b->data;
						fread(&b->data,sizeof(uint8_t),1,fp);
						tmp2=b->data;
						fread(&b->data,sizeof(uint8_t),1,fp);
						tmp3=b->data;

						value=(tmp1 & 0xff) << 0 | (tmp2 & 0xff )<< 8 | ( tmp3 & 0xff )<< 16 | 0x00 << 24;

						printf (" 0x%06X, %s",
								value,
								((j%8) == 7) ? "\n" : ""
						       );
					}
					if (padding) {
						fseek(fp, (long)padding, SEEK_CUR);
					}
				}
				fclose (fp);
				printf ("},\n");
			}
			printf ("\n"
					"};\n\n" );
		}
		printf ("#endif /* __CHARGE_LOGO_H__ */\n");
		return (0);
	}

}
