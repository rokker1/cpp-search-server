#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view str) {
    vector<string_view> result;
    //1. Удалите начало из str до первого непробельного символа, воспользовавшись методом remove_prefix. 
    // Он уберёт из string_view указанное количество символов.
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    //2. В цикле используйте метод find с одним параметром, 
    // чтобы найти номер позиции первого пробела.
    while(str != str.end()) {
        size_t space = str.find(' '); 
        //3. Добавьте в результирующий вектор элемент string_view, полученный вызовом метода substr, 
        // где начальная позиция будет 0, а конечная — найденная позиция пробела или npos.
        result.push_back(space == str.npos ? str.substr(0, str.npos) : str.substr(0, space));
        //4. Сдвиньте начало str так, чтобы оно указывало на позицию за пробелом. Это можно сделать 
        // методом remove_prefix, передвигая начало str на указанное в аргументе количество позиций.
        str.remove_prefix(std::min(str.find_first_not_of(" ", space), str.size()));
    }
    return result;
}