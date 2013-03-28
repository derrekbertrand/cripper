#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>

#define TILE_WIDTH 8 //number of pixels w & h a tile is
#define TILE_SIZE TILE_WIDTH*TILE_WIDTH*sizeof(int) //size of a tile's data in bytes
//macros for getting the local origins
#define origin_x(i, w, t) (i*t%w)
#define origin_y(i, w, t) ((i/(w/t))*t)

typedef struct tile {
    int hash; //hash of tile
    int index; //index of tile in the original file
} tile;

//not worth making a header for one big-ass function
//a function ripped off stack overflow that hashes data
uint32_t lookup3 (const void *key, size_t length, uint32_t initval);

int init_alleg()
{
    if(!al_init())
    {
        printf("Al Init failed!\n");
        return 1;
    }

    if(!al_init_image_addon())
    {
        printf("Al Init Image addon failed!\n");
        return 1;
    }

    if(!al_init_native_dialog_addon())
    {
        printf("Al Init Dialogs failed!\n");
        return 1;
    }

    return 0;
}

//allows the user to select a bitmap with a dialog
//loads the image and returns it, but only
//if it is divisible by tile width (which we need for our purposes)
ALLEGRO_BITMAP* get_image()
{
    ALLEGRO_BITMAP* source_img = NULL;
    const char* value = NULL;
    ALLEGRO_FILECHOOSER* filechooser = NULL;

    filechooser = al_create_native_file_dialog("", "Choose Source Image", "*.png", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST | ALLEGRO_FILECHOOSER_PICTURES);

    //we have a sensible dialog
    if(filechooser != NULL)
    {
        if(al_show_native_file_dialog(NULL, filechooser))
        {
            if(al_get_native_file_dialog_count(filechooser) == 1)
            {
                //we have a path
                value = al_get_native_file_dialog_path(filechooser, 0);

                //we aren't concerned with displaying; we're concerned with
                //manipulating the image in memory
                al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

                //load the img
                source_img = al_load_bitmap(value);

                //check to see if it loaded
                //if not, go ahead and quit here
                if(source_img == NULL)
                {
                    al_show_native_message_box(
                        NULL,
                        "Error",
                        "Error",
                        "That is not a valid PNG file.",
                        NULL,
                        ALLEGRO_MESSAGEBOX_ERROR
                    );
                    return NULL;
                }

                //its nice that we have an image, but we are a bit more particular than that
                //it needs to be divisible by tile size
                if(al_get_bitmap_width(source_img) % TILE_WIDTH)
                {
                    al_show_native_message_box(
                        NULL,
                        "Error",
                        "Error",
                        "The image is not divisible by tile width!",
                        NULL,
                        ALLEGRO_MESSAGEBOX_ERROR
                    );
                    al_destroy_bitmap(source_img);
                    return NULL;
                }

                if(al_get_bitmap_height(source_img) % TILE_WIDTH)
                {
                    al_show_native_message_box(
                        NULL,
                        "Error",
                        "Error",
                        "The image is not divisible by tile height!",
                        NULL,
                        ALLEGRO_MESSAGEBOX_ERROR
                    );
                    al_destroy_bitmap(source_img);
                    return NULL;
                }
            }
        }
        //clean up the file dialog
        al_destroy_native_file_dialog(filechooser);
    }
    return source_img;
}

void output_image(ALLEGRO_BITMAP* src, int w, int h, tile* hashes, int n_unique)
{
    ALLEGRO_BITMAP* dst = NULL;

    //at this point we should have a nice image of proper size loaded
    //make another of that size
    dst = al_create_bitmap(w, h);

    if(dst == NULL)
        return;

    //set drawing target and clear to fuschia
    al_set_target_bitmap(dst);
    al_clear_to_color(al_map_rgb(255,0,255));

    //now we do the transplant
    int i = 0;
    for(i = 0;i < n_unique;i++)
    {
        al_draw_bitmap_region(src,
            origin_x(hashes[i].index,w,TILE_WIDTH), origin_y(hashes[i].index,w,TILE_WIDTH),
            TILE_WIDTH, TILE_WIDTH,
            origin_x(i,w,TILE_WIDTH), origin_y(i,w,TILE_WIDTH), 0);
    }

    //save bitmap as out.png
    al_save_bitmap("out.png", dst);
    //unset target
    al_set_target_bitmap(NULL);
    //destroy
    al_destroy_bitmap(dst);
}

int rip_tiles(ALLEGRO_BITMAP* src)
{
    int unique = 1;
    int i = 0; //index of the tile
    int j = 0; //index of the max unique hashes (to be used with hashes)
    int k = 0; //iterator for use with j
    int temp = 0; //temp storage for hash
    int row = 0;
    int col = 0; //row and column
    int w = al_get_bitmap_width(src);
    int h = al_get_bitmap_height(src);
    int n_tiles = (w/TILE_WIDTH) * (h/TILE_WIDTH); //the number of tiles
    int data[TILE_WIDTH][TILE_WIDTH]; //the data in the tiles
    unsigned char r, g, b, a; //holders for the color elements

    tile* hashes = al_malloc(sizeof(tile)*n_tiles);
    memset ((void*)hashes, 0, sizeof(hashes));

    //iterate through tiles
    for(i = 0;i < n_tiles;i++)
    {
        //dump bitmap data into an array
        for(row = 0;row < TILE_WIDTH;row++)
            for(col = 0;col < TILE_WIDTH;col++)
            {
                al_unmap_rgba(al_get_pixel(src, origin_x(i,w,TILE_WIDTH) + col, origin_y(i,w,TILE_WIDTH) + row), &r, &g, &b, &a);
                data[row][col] = r | (g << 8) | (b << 16) | (a << 24);
            }

        temp = lookup3((const void*)data,sizeof(data),12345);
        unique = 1;
        //check vs all the other unique ones found so far
        for(k = 0; k < j;k++)
        {
            if(hashes[k].hash == temp)
            {
                //not unique!
                unique = 0;
                //break the loop
                break;
            }
        }

        //if its still considered unique
        if(unique)
        {
            //add the hash and index
            hashes[j].hash = temp;
            hashes[j].index = i;
            j++;   //we now have one more unique one
        }
    }

    output_image(src, w, h, hashes, j);
    al_free(hashes);
    return 0;
}

int main()
{
    ALLEGRO_BITMAP* src_img = NULL;

    if(init_alleg())
        return 1;

    src_img = get_image();

    if(src_img == NULL)
        return 1;

    rip_tiles(src_img);

    //destroy the bitmaps and shut down
    al_destroy_bitmap(src_img);
    al_shutdown_native_dialog_addon();
    return 0;
}
