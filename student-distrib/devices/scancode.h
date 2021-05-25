/*refer from this website: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html*/
#ifndef KEYBOARD_SCANCODE_H
#define KEYBOARD_SCANCODE_H

#define SCANCODE_SIZE 0x45
#define CHOICE_SIZE   2

// define VK marco
#define VK_CAPITAL    20
#define VK_F1         112
#define VK_F2         113
#define VK_F3         114
#define VK_F4         115
#define VK_F5         116
#define VK_F6         117
#define VK_F7         118
#define VK_F8         119
#define VK_F9         120
#define VK_F10        121


//fill the corresponding char to scancode_array
unsigned char scancode_array[SCANCODE_SIZE][CHOICE_SIZE] = 
{
       {   0,0   } ,
       {   0,0   } ,
       { '1','!' } ,
       { '2','@' } ,
       { '3','#' } ,
       { '4','$' } ,
       { '5','%' } ,
       { '6','^' } ,
       { '7','&' } ,
       { '8','*' } ,
       { '9','(' } ,
       { '0',')' } ,
       { '-','_' } ,
       { '=','+' } ,
       {   8,8   } ,
       {   0,0   } ,
       { 'q','Q' } ,
       { 'w','W' } ,
       { 'e','E' } ,
       { 'r','R' } ,
       { 't','T' } ,
       { 'y','Y' } ,
       { 'u','U' } ,
       { 'i','I' } ,
       { 'o','O' } ,
       { 'p','P' } ,
       { '[','{' } ,
       { ']','}' } ,
       {'\n','\n'} ,
       {   0,0   } ,
       { 'a','A' } ,
       { 's','S' } ,
       { 'd','D' } ,
       { 'f','F' } ,
       { 'g','G' } ,
       { 'h','H' } ,
       { 'j','J' } ,
       { 'k','K' } ,
       { 'l','L' } ,
       { ';',':' } ,
       {  39, 34 } ,
       { '`','~' } ,
       {   0,0   } ,
       { '\\','|'} ,
       { 'z','Z' } ,
       { 'x','X' } ,
       { 'c','C' } ,
       { 'v','V' } ,
       { 'b','B' } ,
       { 'n','N' } ,
       { 'm','M' } ,
       { ',','<' } ,
       { '.','>' } ,
       { '/','?' } ,
       {   0,0   } ,
       {   0,0   } ,
       {   0,0   } ,
       { ' ',' ' } ,
       {VK_CAPITAL,VK_CAPITAL} ,
       {VK_F1,VK_F1} ,
       {VK_F2,VK_F2} ,
       {VK_F3,VK_F3} ,
       {VK_F4,VK_F4} ,
       {VK_F5,VK_F5} ,
       {VK_F6,VK_F6} ,
       {VK_F7,VK_F7} ,
       {VK_F8,VK_F8} ,
       {VK_F9,VK_F9} ,
       {VK_F10,VK_F10} ,
};


#endif 


