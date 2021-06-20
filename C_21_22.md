# С

> Автор: Воробьев Вячеслав

# Table of Contents
1. [21-210316](##21-210316)
  1. [Массивы в С](###Массивы-в-С)
  1. [Массивы как аргументы функций](###Массивы-как-аргументы-функций)
  1. [Массивы строк](###Массивы-строк)
1. [22-210323](##22-210322)
  1. [Указатели](###Указатели)
  1. [Отличия C и С++](###Отличия-C-и-С++)
  1. [Некоторые идиомы C](###Некоторые-идиомы-C)


## 21-210316

### Массивы в С

Массивы. При инициализации непроинициализированные элементы заполняются дефолтными значениями - default-initialize (с int это будет 0).
```C
int a1[10];
    int a2[10] = {};  // default-initialize всех элементов: 0.
    int a3[10]{};
    int a4[10] = {1, 2, 3};  // default-initialize всех элементов (кроме первых, они copy-initialize): 0.
    int a5[10]{1, 2, 3};
    int a6[10] = { 0 };  // массив нулей.
    int a7[10] = { 1 };  // массив нулей с единицей.
```
Массив char можно инициализировать строковыми литералами.
```C
char c1[] = {1, 2, 3, 4};
    char c2[] = "hello";  // Массив размера 6.
    char c3[10] = "hello";  // Массив размера 10, проинициализированы первые 6 символов.
```
Массивы приводятся к указателю на элемент и с помощью арифметики указателей можно гулять по его элементам. Даже можно взять указатель на элемент после последнего (one-past-the-end), но его нельзя разыменовывать или обращаться к нему - UB.
```C
int *aptr = a1;
    assert(aptr == &a1[0]);
    assert(aptr + 1 == &a1[1]);
    assert(aptr + 9 == &a1[9]);
    aptr + 10;  // one-past-the-end, но нельзя разыменовать.
    // &a1[10];  // UB - обращение к несуществующему элементу массива.
```
`sizeof` - размер в байтах. Если поделить на размер одного элемента, то получим кол-во элементов.
```C
assert(std::size(a1) == sizeof(a1) / sizeof(a1[0]));
```

### Массивы как аргументы функций
Массивы при передаче в функцию преобразовываются к указателю и теряют размер. Его нужно передавать отдельно.
```C
// void foo(int arr[]) {  // То же самое, что int *arr
void foo(int arr[15]) {  // То же самое, что int *arr
    std::cout << sizeof(arr) << "\n"; // 8
}
```
В плюсах можно взять ссылку на массив. Тогда будет проверка, что сходятся размеры.

```C++
void foo_cpp_wtf(int (&arr)[10]) {}  // C++ only, ok
```

Можно функцию сделать шаблонной. Она будет принимать ссылку на массив размера N. N - шаблонный параметр.

```C++
template<std::size_t N>
void foo_cpp_templ(int (&arr)[N]) {
    std::cout << "N=" << N << "\n";
}
```

### Массивы строк
Почти двумерный массив символов. У каждого массива есть свой указатель, а также есть указатель на первый массив. Нужно линейное количество выделений памяти.
```C
//void print_all(const char *arr[]) {
void print_all(const char **arr) {  // примерно vector<vector<char>>, только совсем руками.
    for (int i = 0; arr[i] != nullptr; i++) {
        //std::cout << "i=" << i << ": " << arr[i] << "\n";
        std::cout << "i=" << i << ": ";
        for (int j = 0; arr[i][j] != 0; j++) {
            std::cout << arr[i][j];
        }
        std::cout << "\n";
    }
}

int main() {
    const char *arr[] = {
        "Hello123456",
        "World",
        nullptr  // NULL в языке Си. Чтобы можно было узнать размер
    };
    print_all(arr);
}
```


## 22-210323

### Указатели
Указатели (многомерные тоже) возникают в нескольких местах:
- массив строк, пример чуть выше
- если хотим возвращать значение через аргументы (а в С ссылок нет)
```C++
int create_arr(int **arr, int len) {
    *arr = new int[len];
    for (int i = 0; i < len; i++) {
        (*arr)[i] = 100 + i;
    }
    return 0;
}
int main() {
    int *arr;
    create_arr(&arr, 10);
    std::cout << arr[5] << "\n"; // 105
    delete[] arr;
}
```
- `scanf` - сишная функция, ссылок нет, поэтому передаём указатели
```C
int h, m;
scanf("%d:%d", &h, &m);
printf("%02d:%02d\n", h, m);
```
- Многомерные массивы
Можно просто попросить многомерный массив, тогда элементы лежат подряд.
```C
int arr[3][4][5]{};
```
А можно собрать такое ручками с таким же синтаксисом. Отсюда несколько эффектов: теперь это несколько блоков в памяти, которые могут лежать не подряд, также не все массивы должны быть одинаковой длины, мы сами управляем этим.
```C
int ***arr2 = new int**[3];
    for (int i = 0; i < 3; i++) {
        arr2[i] = new int*[4];
        arr2[i][0] = new int[5];
        arr2[i][1] = new int[5];
        arr2[i][2] = new int[5];
        arr2[i][3] = new int[5];
    }
    arr2[1][2][3] = 123;
```
Также функции могут по-разному принимать такие аргументы:
Просто как многомерный указатель, проподают требования к размерам, но и самих размеров уже нет
```C
void bar(int ***arr) {
    std::cout << arr[1][2][3] << "\n";
}
```
Или принимать именно указатель на массив в разных видах:
```C
using arr45 = int[4][5];
// void foo(int arr[3][4][5]) {
// void foo(int arr[][4][5]) {
// void foo(int (*arr)[4][5]) {  // What if: (*arr) --> *arr
void foo(arr45 *arr) {
    std::cout << arr[1][2][3] << "\n";
}
```
Можно опять же сделать функцию шаблонной:
```C
template<std::size_t N, std::size_t M, std::size_t K>
void foo2(int (&arr)[N][M][K]) {
    std::cout << arr[1][2][3] << "\n";
    std::cout << N << " " << M << " " << K << "\n";
}
```

### Отличия С и C++
- Первый стандарт С89: переменные только в начале функции, неявные include (GCC всё ещё поддерживает, хотя убрали в C99), комментарии только /* wtf */ (вроде нет в билете)
- Если функция объявлена с пустыми скобками, то по умолчанию может принимать любые аргументы, лечится с помощью `void` - не принимает аргументы
```C
void bar() {   // Any arguments!
}
void baz(void) {   // No arguments
}
```
- Чего нет в C:
  * `*_cast` --> C-style-cast.
  * `int a{}` --> `int a = 0;`
  * namespaces
  * references --> pointers
  * templates --> macros
  * functions: overloading, default parameters
  * `bool` --> `int` + `0`/`1` OR `<stdbool.h>`
  * `std::vector`
  * `using` --> `typedef`
  * exceptions
- Структурки, ничего нет: методов, конструкторов/деструкторов, методов, перегрузки операторов, `private/public` Полный тип это: `struct my_struct`. Можно починить с `typedef`
```C
typedef struct point3 {
    int x;
    int y;
} point3, *ppoint3;
```
- В С есть больше неявных конвертаций, например для `void*`, без расширений в C++ не скомпилируется, тк `malloc` возвращает `void*`.
```C
#include <stdlib.h>
int main(void) {
    int *a = malloc(5 * sizeof(int));  // new int[5];
    free(a);  // delete[] a
}
```
Или ещё несо вместимость, в C символьный литерал это `int`
```C
#include <stdlib.h>
#include <stdio.h>
int main(void) {
    // 'A' is int in C, char in C++.
    printf("%d %d\n", (int)sizeof('A'), (int)sizeof(char)); // 4 1
}
```
    int *a = malloc(5 * sizeof(int));  // new int[5];
    free(a);  // delete[] a
    // 2.
- Расширения GCC, очень много
- Макросы, очень много

### Некоторые идиомы
- for-each
```C
#include <stdio.h>
#include <stddef.h>
void for_each(void *begin, void *end, size_t elem_size, void (*f)(const void*)) {
    while (begin != end)
        f(begin += elem_size);  // GCC extension: arithmetics on void*
}
int main(void) {
    int arr[10] = {
        // Designated initializers for arrays.
        [3] = 100,
        [5] = 200
    };
    // Careful: nested function is GCC extension.
    void print_int(int *x) {
        printf("%d\n", *x);
    }

    for_each(arr, arr + 10, sizeof arr[0],
        print_int  // WARNING: invalid conversion. GCC does not even care about 'const'.
    );
}
```
- bubble-sort
```C
#include <stddef.h>
#include <stdio.h>
#include <strings.h>
void swap(char *a, char *b, size_t size) {
    for (; size > 0; a++, b++, size--) {
        char t = *a;
        *a = *b;
        *b = t;
    }
}
// Look up qsort()
void bubble_sort(void *data, size_t num, size_t size, int (*compar)(const void*, const void*)) {
    // compar ~~ strcmp ~~ (a - b): a < b => (<0); a == b => 0; a > b => (>0).
    char *end = (char*)data + num * size;
    for (size_t i = 1; i < num; i++) {
        for (char *a = data, *b = a + size; b != end; a += size, b += size) {
           printf("compare %p %p (%s %s); %d\n", a, b, *(char**)a, *(char**)b, compar(a, b));
           if (compar(a, b) > 0) {
               // Tradition: everything is trivially copiable/movable.
               // Trivially ~ bytewise (a term from C and C++).
               swap(a, b, size);
           }
        }
    }
}
int my_strcmp(const void *a, const void *b) {  // !!! Not just strcmp!
    const char **a_str = a;
    const char **b_str = b;
    return strcmp(*a_str, *b_str);
}
int main() {
    const char *strs[] = {
        "Hello",
        "World",
        "Egor",
        "Suvorov"
    };
    bubble_sort(strs, sizeof strs / sizeof strs[0], sizeof strs[0],
        my_strcmp
    );
    for (int i = 0; i < 4; i++) {
        printf("%s\n", strs[i]);
    }
}
```
