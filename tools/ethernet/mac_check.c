#include <stdio.h>
#include <string.h>
#include <ctype.h>

int bachk(const char *str)
{       
        if (!str)
                return -1;
        
        if (strlen(str) != 17)
                return -1;
        
        while (*str) {
                if (!isxdigit(*str++))
                        return -1;
                
                if (!isxdigit(*str++))
                        return -1;
                
                if (*str == 0)
                        break;
                
                if (*str++ != ':')
                        return -1;
        }
        
        return 0;
}

int main( int argc, char *argv[] )
{
	if(!bachk(argv[1]))
		printf("ok\n");
	else
		printf("error\n");

	return 0;
}
