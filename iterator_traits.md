## iterator_traits

Рассмотрим ситуацию:
```c++
template<typename T>
void print_min_element(T first, [[maybe_unused]] T last) {
    ??? cur_min = *first;
}
```
Что написать вместо `???`?
* Варианты:
* Если есть `auto` - нужно написать `auto`!
* `T*`, то не поможет, так как мы объявили указатель на итератор, это плохо
* Есть вариант:
```c++
template<typename T>
void print_min_element(T first, [[maybe_unused]] T last) {
    decltype(*first) cur_min = *first;

}
```
это **не** то же самое, что `auto`, так как, например, не отбросится ссылка и, следоватлеьно, константность (если передать итераторы на концы константного элемента, то менять `cur_min` по прежнему будет нельзя). И это грустно. 
* Если пишите **не** на C++11, то надо использовать `iterator_traits`:
```c++
template<typename T>
void print_min_element(T first, [[maybe_unused]] T last) {
    // Use in C++03, not in C++11.
    [[maybe_unused]] typename std::iterator_traits<T>::value_type cur_min = *first;
    cur_min = 100;
}
```
* Нужен `typename` так как компилятор не уверен, что `std::iterator_traits<T>::value_type` тип.
* Можно было бы писать:
```c++
typename T::value_type cur_min = *first;
``` 
но это не работает с указателем (например: с массивом - `std::begin(arr)`). И так нельзя писать, если нет `auto`, то `iterator_traits`.
