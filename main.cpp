#include "vector.h"

#include <cassert>
#include <algorithm>
#include <iostream>

template <typename T>
void OutputContainer(const simple_vector::Vector<T> &vec)
{
    for (int num : vec)
    {
        std::cout << num << " "; // output: 40 30 20 10 0
    }
    std::cout << "\n";
}

int main(){

    simple_vector::Vector<int> vec;

    for(int i = 4; i >= 0 ; i--){
        vec.PushBack(i * 10); // {40 , 30 , 20 ,10 , 0}
    }

    OutputContainer(vec); // output: 40 30 20 10 0

    assert(vec.Size() == 5);
   
    std::sort(vec.begin(), vec.end());

    OutputContainer(vec); // output: 0 10 20 30 40

    std::transform(vec.begin(), vec.end(), vec.begin(),
         [](int num) { return num + 5; });

    OutputContainer(vec); // output: 5 15 25 35 45

    return 0;
}