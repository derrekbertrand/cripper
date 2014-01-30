#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <stdint.h>
#include <iostream>
#include <map>

uint8_t magic_r = 255;
uint8_t magic_g = 0;
uint8_t magic_b = 255;
uint8_t tile_width = 8;
const char* default_output_file = "output.png"; //the default
char* output_file = NULL; //points to what will be actually used

/*=============================================================================
Definitions
=============================================================================*/

//convert bmp width, tile width, and a tile index to the x and y origin
//of that tile 
#define ORIGIN_X(i, tw, bw) ((i%(bw/tw))*tw)
#define ORIGIN_Y(i, tw, bw) ((i/(bw/tw))*tw)

//take tile width, x, and y; return the array index it goes in
//also allow simulation of H and V flipping
#define TILE_COORD(x, y, tw) ((y*tw)+x)
#define TILE_COORD_H(x, y, tw) ((y*tw)+(tw-1)-x)
#define TILE_COORD_V(x, y, tw) (((tw-1)*tw)+(y*tw*-1)+x)
#define TILE_COORD_HV(x, y, tw) (((tw-1)*tw)+(y*tw*-1)+(tw-1)-x)

//struct to hold tile data and its hashes
typedef struct{
    uint32_t* data;
    uint32_t hash1;
    uint32_t hash2;
    uint32_t hash3;
    uint32_t hash4;
} tile_data;

//a container type to hold the tiles, and an iterator
typedef std::map<uint32_t, tile_data*> tile_stack;
typedef std::map<uint32_t, tile_data*>::iterator tile_iterator;

//little endian hash function kindly provided by Bob Jenkins in the Public Domain
uint32_t parse_arguments(int nargs, char* args[]);
uint32_t hashlittle( const void *key, size_t length, uint32_t initval);
#define lookup3(key, length) hashlittle(key, length, 0xA11E6705)
tile_data* get_raw_data(ALLEGRO_BITMAP* bmp, uint32_t tile_number);
uint32_t pixel_data_slow(ALLEGRO_BITMAP* bmp, uint32_t x, uint32_t y);
int32_t rip_tiles(ALLEGRO_FS_ENTRY* src, tile_stack* t_stack);
ALLEGRO_BITMAP* consolidate_tiles(tile_stack* t_stack);
void output_bitmap(ALLEGRO_BITMAP* bmp, const char* path);
void draw_tile(uint32_t i, tile_data* t_data);
//=============================================================================

int main(int nargs, char* args[])
{
    tile_stack t_stack; //the map structure of tiles
    uint32_t bitmap_count = 0; //number of bitmaps opened
    uint32_t tile_count = 0; //the number of tiles cumulatively ripped
    int32_t ripped_result = 0; //a temporary variable for the results of rip_tiles
    ALLEGRO_FS_ENTRY* dir = NULL; 
    ALLEGRO_FS_ENTRY* file = NULL;

    //=========================================================================

    //parse arguments
    if(parse_arguments(nargs, args))
        return 1;

    //init the stuffs
    al_init();
    al_init_image_addon();

    //get the directory entry for the current directory
    dir = al_create_fs_entry(".");

    if(dir == NULL)
    {
        std::cout << "Cannot get FS handle for current directory." << std::endl;
        return 1;
    }

    //now that we have a fs entry for dir, open the dir
    if(!al_open_directory(dir))
    {
        std::cout << "Cannot open directory." << std::endl;
        al_destroy_fs_entry(dir);
        return 1;
    }

    //read the files one by one
    while(1)
    {
        //if we have a file release it before getting the next one
        if(file != NULL)
            al_destroy_fs_entry(file);

        //get the next file
        file = al_read_directory(dir);

        //if null then something is wrong or we're done
        if(file == NULL)
            break;

        //We need to have either a BMP or a PNG
        if(strstr(al_get_fs_entry_name(file), ".png") == NULL)
            if(strstr(al_get_fs_entry_name(file), ".bmp") == NULL)
                if(strstr(al_get_fs_entry_name(file), ".PNG") == NULL)
                    if(strstr(al_get_fs_entry_name(file), ".BMP") == NULL)
                        continue;

        //returns number of tiles ripped, or -1 for no-go
        ripped_result = rip_tiles(file, &t_stack);
        if(ripped_result >= 0)
        {
            bitmap_count++;
            std::cout << "On bitmap " << bitmap_count << ", named \"" << al_get_fs_entry_name(file) << "\" :" << std::endl;

            tile_count += ripped_result;
            std::cout << "Ripped " << ripped_result << " new tiles. Current total: " << tile_count << std::endl;
        }
    }
    //announce that we have finished iterating through the bitmaps
    std::cout << "We have finished iterating through the bitmaps.\n" << std::endl;

    //get rid of the tile
    output_bitmap(consolidate_tiles(&t_stack), output_file);

    //clean up after yourself
    if(dir != NULL)
    {
        al_close_directory(dir);
        al_destroy_fs_entry(dir);
    }
    if(file != NULL)
    {
        al_destroy_fs_entry(file);
    }
    return 0;
}

uint32_t parse_arguments(int nargs, char* args[])
{
    //go through each of the arguments
    for(uint32_t i = 1; i < nargs; i++)
    {
        //is it a tile width?
        if((strcmp(args[i], "-tw") == 0))
        {
            i++; //working with this value

            if(i >= nargs)
            {
                std::cout << "Missing argument for -tw." << std::endl;
                return 1;
            }


            tile_width = strtol(args[i], NULL, 10);

            //check it
            if(tile_width == 0)
            {
                std::cout << args[i] << " does not work as an argument to -tw." << std::endl;
                return 1;
            }
        }
        else if((strcmp(args[i], "-o") == 0))
        {
            i++; //working with this value

            if(i >= nargs)
            {
                std::cout << "Missing argument for -o." << std::endl;
                return 1;
            }

            if(strlen(args[i]) < 4)
            {
                std::cout << args[i] << " does not work as an argument to -o." << std::endl;
                return 1;
            }

            output_file = args[i];
        }
    }

    //clean up

    //we need to make sure we have an output file
    if(output_file == NULL)
    {
        output_file = (char*)default_output_file;
    }

    return 0;
}

//rip_tiles expects the FS entry for the file in question and a pointer to a tile stack
//it will add the unique tiles to the tile stack, and return the number added
//negative means something went wrong with the image (not an image, not right size, etc)
int32_t rip_tiles(ALLEGRO_FS_ENTRY* src, tile_stack* t_stack)
{
    uint32_t tile_count = 0;
    ALLEGRO_BITMAP* bmp = NULL;
    tile_data* t_data = NULL;


    //take entry and open BMP in exactly the way the documentation says not to
    bmp = al_load_bitmap_flags(al_get_fs_entry_name(src), ALLEGRO_NO_PREMULTIPLIED_ALPHA);

    //the bitmap wouldn't open
    if(bmp == NULL)
        return -1;

    //the bitmap must be divisible by tile_width
    if(al_get_bitmap_width(bmp) % tile_width || al_get_bitmap_height(bmp) % tile_width)
    {
        std::cout << "The bitmap, " << al_get_fs_entry_name(src) << ", is not the proper size." << std::endl;
        return -1;
    }

    //for each tile, go through and check it vs the others
    for(int i = 0; i < ((al_get_bitmap_width(bmp)/tile_width)*(al_get_bitmap_height(bmp)/tile_width));i++)
    {
        t_data = get_raw_data(bmp, i);

        //if not null, do stuff and delete
        if(t_data != NULL)
        {
            //check the tile vs others
            if(t_stack->find(t_data->hash1) != t_stack->end())
            {
                //delete data and the tile data
                al_free(t_data->data);
                al_free(t_data);
            }
            else if(t_stack->find(t_data->hash2) != t_stack->end())
            {
                //delete data and the tile data
                al_free(t_data->data);
                al_free(t_data);
            }
            else if(t_stack->find(t_data->hash3) != t_stack->end())
            {
                //delete data and the tile data
                al_free(t_data->data);
                al_free(t_data);
            }
            else if(t_stack->find(t_data->hash4) != t_stack->end())
            {
                //delete data and the tile data
                al_free(t_data->data);
                al_free(t_data);
            }
            //the tile is not in the stack
            else
            {
                t_stack->insert(std::pair<uint32_t, tile_data*>(t_data->hash1, t_data));
                std::cout << "    Inserted tile number " << i << std::endl;
                tile_count++;
            } 
            
        }
        else
        {
            std::cout << "    Got null from get_raw_data() on tile " << i << ". Skipping the rest." << std::endl;
            break;
        }
    }

    al_destroy_bitmap(bmp);

    //return the number of tiles
    return tile_count;
}

tile_data* get_raw_data(ALLEGRO_BITMAP* bmp, uint32_t tile_number)
{
    tile_data* t_data = NULL;
    //get the x and y origin of the tile in question
    uint32_t origin_x = ORIGIN_X(tile_number, tile_width, al_get_bitmap_width(bmp));
    uint32_t origin_y = ORIGIN_Y(tile_number, tile_width, al_get_bitmap_width(bmp));

    //check bitmap bounds just in case
    if((tile_width+origin_x) > al_get_bitmap_width(bmp))
    {
        std::cout << origin_x+tile_width << " is outside the X bounds of the bitmap." << std::endl;
        return NULL;
    }

    if((tile_width+origin_y) > al_get_bitmap_height(bmp))
    {
        std::cout << origin_y+tile_width << " is outside the Y bounds of the bitmap." << std::endl;
        return NULL;
    }

    //allocate tile data structure
    t_data = (tile_data*)al_malloc(sizeof(tile_data));
    
    if(t_data == NULL)
    {
        std::cout << "Couldn't allocate " << sizeof(tile_data) << " bytes." << std::endl;
        return NULL;
    }
    else
    {
        //allocate the tile's pixel data
        t_data->data = (uint32_t*)al_malloc(sizeof(uint32_t)*tile_width*tile_width);

        //we are making 2 allocations, so we need 2 checks
        if(t_data->data == NULL)
        {
            std::cout << "Couldn't allocate " << sizeof(uint32_t)*tile_width*tile_width << " bytes." << std::endl;
            al_free(t_data);
            return NULL;
        }

    }

    //we have a tile within bounds and the memory allocated to do processing on it

    
    //=========================================================================
    //iterate through all the pixels and pack them into the data array
    for(int32_t y = 0; y < tile_width; y++)
        for(int32_t x = 0; x < tile_width; x++)
        {
            t_data->data[TILE_COORD_HV(x, y, tile_width)] = pixel_data_slow(bmp, origin_x+x, origin_y+y);
        }

    //get the hash
    t_data->hash4 = lookup3(t_data->data, sizeof(uint32_t)*tile_width*tile_width);


    //=========================================================================
    //iterate through all the pixels and pack them into the data array
    for(int32_t y = 0; y < tile_width; y++)
        for(int32_t x = 0; x < tile_width; x++)
        {
            t_data->data[TILE_COORD_H(x, y, tile_width)] = pixel_data_slow(bmp, origin_x+x, origin_y+y);
        }

    //get the hash
    t_data->hash3 = lookup3(t_data->data, sizeof(uint32_t)*tile_width*tile_width);


    //=========================================================================
    //iterate through all the pixels and pack them into the data array
    for(int32_t y = 0; y < tile_width; y++)
        for(int32_t x = 0; x < tile_width; x++)
        {
            t_data->data[TILE_COORD_V(x, y, tile_width)] = pixel_data_slow(bmp, origin_x+x, origin_y+y);
        }

    //get the hash
    t_data->hash2 = lookup3(t_data->data, sizeof(uint32_t)*tile_width*tile_width);


    //=========================================================================
    //iterate through all the pixels and pack them into the data array
    for(int32_t y = 0; y < tile_width; y++)
        for(int32_t x = 0; x < tile_width; x++)
        {
            t_data->data[TILE_COORD(x, y, tile_width)] = pixel_data_slow(bmp, origin_x+x, origin_y+y);
        }

    //get the hash
    t_data->hash1 = lookup3(t_data->data, sizeof(uint32_t)*tile_width*tile_width);

    //we leave the last configuration there. We now have the tile data, in the order on the bitmap
    //and four hashes of it to compare to other tiles

    return t_data;
}

uint32_t pixel_data_slow(ALLEGRO_BITMAP* bmp, uint32_t x, uint32_t y)
{
    uint8_t r,g,b,a = 0;

    //map pixel at x and y to rgba
    al_unmap_rgba(al_get_pixel(bmp, x, y), &r, &g, &b, &a);

    //check for magic pink or fully transparent
    if(a == 0)
        return (magic_r<<24)|(magic_g<<16)|(magic_b<<8);

    if(r == magic_r && g == magic_g && b == magic_b)
        return (magic_r<<24)|(magic_g<<16)|(magic_b<<8);

    //return data packed in RGBA format
    return (r<<24)|(g<<16)|(b<<8)|a;
}

ALLEGRO_BITMAP* consolidate_tiles(tile_stack* t_stack)
{
    ALLEGRO_BITMAP* bmp = NULL;
    uint32_t bmp_width = 0;
    uint32_t bmp_height = 0;
    uint32_t t_count = 0;
    uint32_t i = 0; //iterating through the map doesn't actually give us an index of any kind

    t_count = t_stack->size();
    std::cout << "Writing " << t_count << " tiles to a bitmap." << std::endl;

    bmp_width = 32*tile_width;
    bmp_height = t_count/32;
    //if it doesn't fit into 32 quite right, add another row
    if(t_count % 32)
        bmp_height += 1;
    bmp_height *= tile_width;

    //create the bitmap and clear
    bmp = al_create_bitmap(bmp_width, bmp_height);
    if(bmp == NULL)
        return NULL;
    al_set_target_bitmap(bmp);
    al_clear_to_color(al_map_rgba(magic_r, magic_g, magic_b, 0));

    //take each tile: draw it to the bmp, delete it's data
    for(tile_iterator it = t_stack->begin();it != t_stack->end(); ++it)
    {
        //draw tile to the target bitmap at index i
        draw_tile(i, it->second);

        //std::cout << it->first << std:: endl;

        //delete the tile data
        al_free(it->second->data);
        al_free(it->second);

        //increment the tile index
        i++;
    }

    //empty the stack
    t_stack->erase(t_stack->begin(),t_stack->end());
    return bmp;
}

void output_bitmap(ALLEGRO_BITMAP* bmp, const char* path)
{
    std::cout << "Attempting to output " << path << std::endl;

    if((al_get_bitmap_height(bmp) && al_get_bitmap_width(bmp)) != 0)
        if(!al_save_bitmap(path, bmp))    //This actually saves; not just an if statement
            std::cout << "Couldn't save bitmap!" << std::endl;
}

void draw_tile(uint32_t i, tile_data* t_data)
{
    uint32_t bw = 32*tile_width;
    uint8_t r,g,b,a = 0;
    uint32_t origin_x = ORIGIN_X(i, tile_width, bw);
    uint32_t origin_y = ORIGIN_Y(i, tile_width, bw);

    for(uint32_t x = 0; x < tile_width; x++)
        for(uint32_t y = 0; y < tile_width; y++)
        {
            r = (0xff000000 & t_data->data[TILE_COORD(x, y, tile_width)]) >> 24;
            g = (0x00ff0000 & t_data->data[TILE_COORD(x, y, tile_width)]) >> 16;
            b = (0x0000ff00 & t_data->data[TILE_COORD(x, y, tile_width)]) >> 8;
            a = 0x000000ff & t_data->data[TILE_COORD(x, y, tile_width)];
            
            al_put_pixel(x+origin_x, y+origin_y, al_map_rgba(r,g,b,a));
        }
}