#include <abi.h>
#include <fudge.h>

void main(void)
{

    file_open(FILE_P0);
    call_unload(FILE_P0);
    file_close(FILE_P0);

}

