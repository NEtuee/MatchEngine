#include <iostream>

int main(int, char**) {
    std::cout << "Hello, world!\n";

    int i = 0;
    ++i;

    std::cout << sizeof(void*) << std::endl;

    system("pause");

    return 0;
}
