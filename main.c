//-----------------------------------------------------------------------------
//             _                       
//    ___ _ __(_)_ __  _ __   ___ _ __ 
//   / __| '__| | '_ \| '_ \ / _ \ '__|
//  | (__| |  | | |_) | |_) |  __/ |   
//   \___|_|  |_| .__/| .__/ \___|_|   
//              |_|   |_|              
// A crappy tile ripper.
// Copyright (C) 2013 talmina@talmina.org
// See README.md for license.
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>

//number of pixels w & h a tile is
#define TILE_WIDTH 8
//size of a tile's data in bytes
#define TILE_SIZE TILE_WIDTH*TILE_WIDTH*sizeof(uint32_t)

//macros for getting the local origins
#define origin_x(i, w) (i*TILE_WIDTH%w)
#define origin_y(i, w) ((i/(w/TILE_WIDTH))*TILE_WIDTH)

typedef struct tile {
    uint32_t hash; //hash of tile used for comparisons
    uint32_t data[TILE_WIDTH][TILE_WIDTH]; //raw data to be written back to a file later
    void* next; //pointer to the next tile in the list
} tile;

//not worth making a header for one big-ass function
//a function ripped off stack overflow that hashes data
uint32_t lookup3 (const void *key, size_t length, uint32_t initval);

void disp_help()
{
    printf("This is the help for the program, but it ain't that helpful.\n");
}

//Initializes allegro and its addons returns 0 if all is okay
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

    return 0;
}

void shutdown_alleg()
{
    return;
}

/*void output_image(ALLEGRO_BITMAP* src, int w, int h, tile* hashes, int n_unique)
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
            origin_x(hashes[i].index,w), origin_y(hashes[i].index,w),
            TILE_WIDTH, TILE_WIDTH,
            origin_x(i,w), origin_y(i,w), 0);
    }

    //save bitmap as out.png
    al_save_bitmap("out.png", dst);
    //unset target
    al_set_target_bitmap(NULL);
    //destroy
    al_destroy_bitmap(dst);
}*/

//the current version that is being worked on
//takes a linked list of tiles and returns
//the number of tiles added
int rip_tiles(tile* tile_list, char* filename)
{
    //the source image
    ALLEGRO_BITMAP* img = NULL;
    ALLEGRO_LOCKED_REGION* region = NULL;
    //iterators
    uint32_t i;
    //row & col
    uint32_t row = 0;
    uint32_t col = 0;
    //width and height
    uint32_t w, h;
    //number of tiles in the image
    uint32_t n_tiles = 0;
    //number of unique tiles
    uint32_t tile_count = 0;
    //the four hashes
    uint32_t hashes[4];
    //the raw data to be hashed in a contiguous region
    uint32_t data[TILE_WIDTH][TILE_WIDTH];
    //the offset into data
    int offset;
    unsigned char* region_data;

    printf("Ripping tiles from %s\n", filename);

    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    //try to load the bmp
    img = al_load_bitmap(filename);

    if(img == NULL)
        return 0;

    w = al_get_bitmap_width(img);
    h = al_get_bitmap_height(img);

    if(w % TILE_WIDTH)
        return 0;

    if(h % TILE_WIDTH)
        return 0;

    //get the number of tiles
    n_tiles = (w/TILE_WIDTH) * (h/TILE_WIDTH);
    //lock the region - er, well... the whole thing
    region = al_lock_bitmap(img, ALLEGRO_PIXEL_FORMAT_ARGB_8888,ALLEGRO_LOCK_READONLY);
    region_data = (unsigned char*)region->data;

    //we now apparently have an image worth working on
    //iterate through each tile
    for(i = 0;i < n_tiles;i++)
    {
        //dump bitmap data into an array
        for(row = 0;row < TILE_WIDTH;row++)
            for(col = 0;col < TILE_WIDTH;col++)
            {
                offset = (region->pitch*(origin_y(i,w)+7-row))+(sizeof(uint32_t)*(origin_x(i,w)+col));
                data[row][col] = region_data[offset];
                printf("%d",data[row][col]);
            }
        hashes[1] = lookup3((const void*)data,sizeof(data),12345);

        /*for(row = 0;row < TILE_WIDTH;row++)
            for(col = 0;col < TILE_WIDTH;col++)
            {
                data[row][col] = (uint32_t)(region->data[(region->pitch*(origin_y(i,w)+row))+(sizeof(uint32_t)*(origin_x(i,w)+7-col))]);
            }
        hashes[2] = lookup3((const void*)data,sizeof(data),12345);

        for(row = 0;row < TILE_WIDTH;row++)
            for(col = 0;col < TILE_WIDTH;col++)
            {
                data[row][col] = (uint32_t)(region->data[(region->pitch*(origin_y(i,w)+7-row))+(sizeof(uint32_t)*(origin_x(i,w)+7-col))]);
            }
        hashes[3] = lookup3((const void*)data,sizeof(data),12345);

        for(row = 0;row < TILE_WIDTH;row++)
            for(col = 0;col < TILE_WIDTH;col++)
            {
                data[row][col] = (uint32_t)(region->data[(region->pitch*(origin_y(i,w)+row))+(sizeof(uint32_t)*(origin_x(i,w)+col))]);
            }
        hashes[0] = lookup3((const void*)data,sizeof(data),12345);

        tile* tile = tile_list;
        while(1)
        {
            //if it matches any of the 4
            //then its not unique, so quit
            if(hashes[0] == tile->hash)
                break;
            if(hashes[1] == tile->hash)
                break;
            if(hashes[2] == tile->hash)
                break;
            if(hashes[3] == tile->hash)
                break;
            
            //alright this tile passed the tests
            //and the next one don't exist
            if(tile->next == NULL)
            {
                //that means its unique
                //allocate a tile and set next to it
                tile->next = al_malloc(sizeof(tile));
                //zero it out
                memset((void*)tile->next, 0,sizeof(tile));
                //copy one of the hashes over
                tile->next->hash = hashes[0];
                //copy the data
                memcpy((void*)tile->next->data, (const void*)data,sizeof(data));

                //we found a tile
                tile_count++;
                //break the loop
                break;
            }
            //not the end try the next one
            tile = tile->next;
        }
        //next tile!*/
    }

    //unlock bitmap
    al_unlock_bitmap(img);
    //destroy the bitmap
    al_destroy_bitmap(img);

    return tile_count;
}

int main(int nargs, char* args[])
{
    //iterators
    uint32_t i;
    //the number of images passed to the program
    uint32_t n_images = 0;
    //the number of tiles we have found
    uint32_t n_tiles = 0;
    //pointer to the tile list
    tile* tile_list = NULL;

    //we loop through the images until we see
    //an argument or the end of the list
    for(i = 0;i < nargs;i++)
    {
        //if the first character is a dash
        //then thats the start of the argument list
        if(args[i][0] == '-')
            break;

        //we have this many images
        n_images = i;
    }

    //display help and exit if no imgs
    if(n_images == 0)
    {
        disp_help();
        return 0;
    }

    //initialize allegro
    if(init_alleg())
        return 1;

    //we have images apparently
    //go through and rip the tiles from each
    //of the images and keep track of the tile count
    for(i = 0;i < n_images;i++)
        n_tiles += rip_tiles(tile_list, args[i]);


    //generate the dump file
    //dump_tiles(n_tiles, tile_list);

    //shut down
    shutdown_alleg();
    return 0;
}
