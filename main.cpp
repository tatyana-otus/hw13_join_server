#include "functions.h"


int main(int argc, char** argv)
{
    try {    
        if (argc != 2){
            std::cerr << "Usage: join_server <port>" << std::endl;
            return 1;
        }
        process(argv[1]);
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
