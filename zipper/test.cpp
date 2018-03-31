#include <iostream>
#include "zipper.hpp"

using namespace std;

int main()
{
    zipper<int> z(-1);
    
    try
    {
        for (int i = 0; i < 26; ++ i)
            if (i % 6 == 5)
                z.step_back(5);
            else
                z.entre_new_branch(i);
    }
    catch(std::exception& except)
    {
        std::cerr << "Exception: " << except.what() << std::endl;
    }

    z.printTree(std::cout);

    return 0;
}
