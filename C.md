# C

## Table of Contents
1. [Арифметика-указателей](#Арифметика-указателей)
1. [Применение-`void*`,-арифметика-с-`void*`](#Применение-`void*`,-арифметика-с-`void*`)
1. [C-style-arrays/массивы-в-стиле-Си](#C-style-arrays/массивы-в-стиле-Си)
1. [Указатели-на-указатели-и-глубже](#Указатели-на-указатели-и-глубже)
1. [Разные-массивы](#Разные-массивы)
1. [`reinterpret_cast`,-strict-aliasing-rule-и-его-нарушения,-корректный-type-punning](#`reinterpret_cast`,-strict-aliasing-rule-и-его-нарушения,-корректный-type-punning)
1. [Plain-Old-Data-(POD)](#Plain-Old-Data-(POD))
1. [C-style-strings/ASCIIZ-строки/строки-в-стиле-Си](#C-style-strings/ASCIIZ-строки/строки-в-стиле-Си)
1. [`const_cast`](#`const_cast`)
1. [Отличия-от-языка-Си](#Отличия-от-языка-Си)
1. [Языковые-особенности-Си](#Языковые-особенности-Си)
1. [Идиомы-языка-Си](#Идиомы-языка-Си)
1. [Линковка-программ-на-Си/C++](#Линковка-программ-на-Си/C++)
1. [Оборачивание-C-API-в-C++-при-помощи-RAII-и-исключений](#Оборачивание-C-API-в-C++-при-помощи-RAII-и-исключений)


## Арифметика-указателей

```C++
struct Base {
    int x, y;
};

int main() {
    Base b[4]{
       {100, 200},
       {300, 400},
       {500, 600},
       {700, 800}
    };
    //будет вывод адрессов на элементы
    std::cout << &b << "\n"; //адресс массива и [0] элемента - совпадают
    std::cout << &b[0] << "\n";
    std::cout << &b[1] << "\n";
    std::cout << &b[2] << "\n";

    Base *bptr = &b[0]; //заводим указатель и вручкую сдвигаем его
                        //не обязательно его заводить на первый элемент
    std::cout << bptr << "\n";
    std::cout << bptr + 1 << "\n";
    std::cout << bptr + 2 << "\n";
}
```

**[]** - синтаксический сахар над арифметикой указателей `a[b] ~~ *(a+b)`
```C++
Base *bptr2 = &b[1];
std::cout << bptr2 - 1 << "\n";
std::cout << bptr2 << "\n";
std::cout << bptr2 + 1 << "\n";

std::cout << bptr2[0].x << " " << bptr2[0].y << "\n";  // (*(bptr2 + 0)).x ~~ (*&b[1]).x --- 300 400
std::cout << bptr2[-1].x << " " << bptr2[-1].y << "\n";  // (*(bptr2 - 1)).x ~~ (*(&b[1] - 1)).x ~~ (*&b[0]).x --- 100 200
```

страшилки (не нужно так писать)
```C++
std::cout << (-1)[bptr2].x << "\n";  // (*(-1 + bptr2)).x ---100
std::cout << 1[bptr2].x << "\n";  // (*(1 + bptr2)).x --- 500
```

Указатель массива совпадает с указателем на первый элемент, но типы это разные (**нельзя**, например, присвоить)
```C++
Base *bp1 = &b;  // не компилируется: &b - это "указатель на массив", а не "указатель на элемент".
```

### One-past-the-last

```C

int main() {
    int arr[4];

    int *x0 = &arr[0];  // OK
    int *x3 = &arr[3];  // OK
    int *x4 = &arr[4];  // UB: не существует arr[4].

    int *p = arr; //уже знаем, что это неявное преобразование (array-to-pointer decay)
    int *p0 = p + 0;
    int *p3 = p + 3;
    int *p4 = p + 4;  // OK: указатель на one-past-last брать можно. Но разыменовывать нельзя.
    *p4;  // UB
    int *p5 = p + 5;  // UB (как только получили выход за пределы, с этого момента началось UB)
    int *p5b = p + 5 - 1;  // UB: (p + 5) - 1
    int *p5c = p + (5 - 1);  // OK: p + 4
    int *p0b = p - 1 + 1;  // UB: (p - 1) + 1
}
```

### Array-to-pointer decay
можно считать, что это неявное преобразование
```C++
    Base *bp2 = b; //но вот так уже делать можно, wtf

/*
пример:
 int arr[]{1, 2, 3, 4};
 std::sort(arr, arr + 4);
неявная конвертация arr в указатель на 1 элемент, и указатель на после последнего, потом сортилось
*/

    auto *bp3 = b;
auto bp4 = b;  // auto тоже делает array-to-pointer decay; массивы-то копировать нельзя.

std::cout << boost::core::demangle(typeid(bp2).name()) << "\n"; // Base*
std::cout << boost::core::demangle(typeid(bp3).name()) << "\n"; // Base*
std::cout << boost::core::demangle(typeid(bp4).name()) << "\n"; // Base*

```

## Применение-`void*`,-арифметика-с-`void*`

`void *` - указатель куда-то, разыменовывать нельзя - CE (compile error)

любой указатель можно -> void \*, но чтобы сделать обратный переход, нужен `static_cast`

Зачем - для полиморфизма

Применение в C++ - [аллокатор памяти] нужно выделить кусок памяти, но не создавать объекты на нем (использовалось, например, в `lab07-vector`, когда мы хотели создать объект, а выделять память по мере необходимости - для этого нужно было разделить 2 действия - выделения памяти и создание объекта (то что совмещает в себе new/delete))

```C++
 // В C++ почти не используется, используется в C.
    void *x = arr;
    // *x;
//    static_cast<int*>(x)
    x++;  // O_O: расширение GCC: работает, как с char* (есть арифметика указателей, но по стандарту так делать нельзя!)

```
У GCC есть расширение для C, которое позволяет завести арифметику на `void*`, работает как char, те можно гулять по байтикам. Такое существует для удобства, тк и так всем ясно, куда он указывает. Если указан флаг -pedantic-errors, то такая арифметика пудеь вызывать ошибку. В C++ так делать нельзя.

## C-style-arrays/массивы-в-стиле-Си

### Массивы

Массивы. Размер нужно знать при компиляции. При инициализации непроинициализированные элементы заполняются дефолтными значениями - default-initialize (с int это будет 0).
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

## Указатели-на-указатели-и-глубже

  Конструкции далее есть и в C и в C++. Начнем разбираться.

  Пусть мы имеем такое наследование:

  [Код](https://github.com/hse-spb-2020-cpp/lectures/blob/master/23-210408/01-language/01-p2p-base.cpp)
  ```C++
  struct Base {};
  struct Derived : Base {};
  struct Derived2 : Base {};

  int main() {
    Base b;
    Derived d;
    Derived2 d2;
  }
  ```

  Далее рассмотрим что мы можем делать с указателями, а что нет:

  * Заведем указетель на `Base` и указатель на `Derived`

    ```C++
    Base *pb = &b; // указатель на Base
    Derived *pd = &d; // указатель на derived
    ```
      * `pb = pd;`

        Мы можем присвоить в указатель на `base` указатель на `derived` потому что в любом наследнике в начале есть базовый класс. Взяли и проинтерпретировали в указатель на что-то меньшего размера.

      * `pd = pb;`

      Мы НЕ можем присвоить в указатель на `derived` указатель на `base`. Воообще говоря не любой `base` является `derived`. Приведем пример, когда разрешение этого приведет к странному исходу

      ```C++
      /// Причина:
          pb = &d2;  // Base* <-- Derived2*
          pd = pb;
          // pd указывает на Derived2, WTF.
      ```

  * Заведем указетель на указатель на `Base` и указатель на указатель на `Derived`

    ```C++
    Base *pb = &b; // указатель на Base
    Base **ppb = &pb;
    Derived *pd = &d; // указатель на derived
    Derived **ppd = &pd;
    ```

      * `ppd = ppb;`

            Мы НЕ можем присвоить в указатель на указатель на `derived` указатель на указатель на `base` потому что, идеалогически, `ppd` может указывать на и на `derived2`. Приведем пример, когда разрешение этого приведет к странному исходу.

            ```C++
            /// Причина:
                pb = &d2;
                // *ppb == &d2
                ppd = ppb;
                // *ppd == &d2, WTF.
            ```

      * `ppb = ppd;`

          Мы НЕ можем присвоить в указатель на указатель на `base` указатель на указатель на `derived`. Странно, да? Приведем пример, когда разрешение этого приведет к странному исходу.

          ```C++
          /// Причина:
              ppb = ppd;
              *ppb = &d2;
              // *ppb == &d2
              // *ppd == &d2, WTF.
          ```
          Пусть мы тогда знаем, что с чтением нет проблем. Могут быть проблемы с записью. То есть через указатель на



### Константность и наследование
TODO

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

### Многомерные массивы
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
Просто как многомерный указатель, пропадают требования к размерам, но и самих размеров уже нет
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
```C++
template<std::size_t N, std::size_t M, std::size_t K>
void foo2(int (&arr)[N][M][K]) {
    std::cout << arr[1][2][3] << "\n";
    std::cout << N << " " << M << " " << K << "\n";
}
```

### Передача параметров из возврата
Если `return` занят под код ошибки или ещё что-то, то возвращаемое значение можно передавать через аргумент по указателю, тогда могут возникать указатели на указатель
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


## Разные-массивы

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

Аналогиично с многомерными массивами, функции могут по-разному принимать такие аргументы:
Просто как многомерный указатель, пропадают требования к размерам, но и самих размеров уже нет
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
```C++
template<std::size_t N, std::size_t M, std::size_t K>
void foo2(int (&arr)[N][M][K]) {
    std::cout << arr[1][2][3] << "\n";
    std::cout << N << " " << M << " " << K << "\n";
}
```


## `reinterpret_cast`,-strict-aliasing-rule-и-его-нарушения,-корректный-type-punning

### Aliasing

**Aliasing** (псевдонимы/наложение/алиасинг) - это ситуация, когда два разных имени (например указателя) обозначают один и тот же объект.

#### Reinterpret-cast

```C
int x = 123456;
char *xc = reinterpret_cast<char*>(&x);
// Разворачивается в такую штуку:
// char *xc = static_cast<char*>(static_cast<void*>(&x));
// один 'static_cast' тут бы не помог
for (std::size_t i = 0; i < sizeof(x); i++) {
    //выводим байты
    std::cout << static_cast<int>(xc[i]) << "\n";
}
```

Пример того, как NOT OK - нельзя смотреть на объекты через указатель другого типа (исключение -char)
```C
float f = 1.0;
static_assert(sizeof(float) == sizeof(int));

int *x = reinterpret_cast<int*>(&f);
std::cout << std::hex << *x /* UB */ << "\n";

// Strict aliasing rule:
// Можно через указатель p типа T1 обращаться к объекту типа T2 только если:
// 1. T1 == T2 (но T1 может быть более const)
// 2. T1 --- базовый класс T2
// .....
// 10. T1 == char

// Выше нарушаем: T1 == int, T2 == float.

// Тут тоже нарушение с точки зрения C++: https://en.wikipedia.org/wiki/Fast_inverse_square_root
// Но, возможно, не с точки зрения C.

```

Зачем такой UB  нужно - очев для оптимизация (wtf 2.0)

Как тут работает strict aliasing rule - он оптимизирует вывод в `return 10`, при этом гарантирует, что `*b`  никак не поменяет a, то есть нельзя к нему обратиться через `b`.
```C
int func(int *a, float *b) {
   *a = 10;
   *b = 123.45;
   return *a;  // --> return 10;
}
```

Пример нарушения:
Тут отработает корректно и вывод будет `10, 10, 123.45`
```C
{
    int a = 15;
    float b = 456.78;
    int res = func(&a, &b);
    std::cout << "res=" << res << "\n";
    std::cout << "a=" << a << "\n";
    std::cout << "b=" << b << "\n";
}

```
А вот тут в `a` запишется полная дичь, потому что `reinterpret_cast` как раз нарушает aliasing
```C
{
    int a = 15;
    int res = func(&a, reinterpret_cast<float*>(&a));
    std::cout << "res=" << res << "\n";
    std::cout << "a=" << a << "\n";
}
```

Но если мы заменим `float` -> `char`, то работать будет как в примере с выводом байт, тк через указатель на char можно обращаться к чему угодно

Можно еще менять байтики
```C
int x = 123456;
char *xc = reinterpret_cast<char*>(&x);
static_assert(sizeof(int) == 4);

xc[0] = 10;
xc[1] = 11;
xc[2] = 12;
xc[3] = 13;
std::cout << std::hex << x << "\n";
```

А так делать плохо, потому что конвертируем в `int`, а на самом деле это массив `char`
```C
char xc[] = {10, 11, 12, 13};
static_assert(sizeof(int) == 4);

int *xptr = reinterpret_cast<int*>(xc);
std::cout << std::hex << *xptr /* UB */ << "\n";
// T1 == int, T2 == char. Нельзя.
```

**Вывод** - через `char` можно объекты менять и читать, но читать `char` как объекты - нельзя

Пример правильно конвертации `float` -> `int` (важно, что это `char*`) (аналог `std::memcpy`)
```C
float x = 1.0;
int y;

static_assert(sizeof(x) == sizeof(y));
// Аналог std::memcpy. Не UB.
// Начиная с C++20 есть bit_cast<>.
for (int i = 0; i < 4; i++) {
    reinterpret_cast<char*>(&y)[i] = reinterpret_cast<char*>(&x)[i];
}

std::cout << std::hex << y << "\n";
```

## Plain-Old-Data-(POD)

Нет виртуальности, нет нетривиальных конструкторов/деструкторов/полей/базовых классов.
Простые базовые классы могут быть.

Применение - в `C` сохраняют данные на диск

Пример такой структурки
```C
struct MyPod {
    int x = 10;
    char y = 20;
    // Компилятор добавил padding: 3 левых байта.
    // А мог и не добавлять (зависит от компилятора)
    float z = 30;
};

int main() {
    MyPod p;
    //создаем бинарный файл
    std::ofstream f("01.bin", std::ios_base::out | std::ios_base::binary);
    // Не UB.
    //передаю указатель "что записывать" и "сколько"
    f.write(reinterpret_cast<const char*>(&p), sizeof(p));
    //аналогично можно считать данные из файла в стурктурку
}
```

В файл запишется `0A 00 00 00 14 10 28 FF | 00 00 F0 41`
Непонятно откуда взялись `10 28 FF`, тк в 14 - это 20, а в конце float -> добавилось выравнивание до 4 байт (можно установить свое, написав перед переменными `alignas(8)`)


У компиляторов есть стек параметров выравнивания, в который можем сами сказать, сколько и чего с помощью `#pragma pack(push, 1)` - выравнивай в этой блоке все по 1-ому байту `#pragma pop()`

```C
#pragma pack(push, 1)
struct MyPod {
    int x = 10;
    char y = 20;
    float z = 30;
};
#pragma pack(pop)

```
Теперь нет padding, меньше зависим от ключей компиляции

Но всё ещё проблемы с:
1. Порядок байт (x86 little-endian; старые Mac, некоторые роутеры на 'mips', но не 'mipsel').
2. Не все процессоры умеют читать невыровненную память (ARM, но я не специалист).
3. Точные размеры int/float - могут быть разными

Про `std::string` - работать не будет в такой структуре, это не тривиальный тип и непонятно куда указатели у него

На такой пример будут ругаться санитайзеры, тк мы невыравненную структурку передаём по ссылке. И компилятор может ожидать (или не может), что оно будет выравненно, и будет этим пользоваться. Что-то типо UB. (Кажется всем пофиг, оно ещё плохо воспроизводится).

```C++
#pragma pack(push, 1)
struct Point {
    char x = 1;
    int y = 2;
};
#pragma pack(pop)

void swap(Point & lhs, Point & rhs) {
    std::swap(lhs.x, rhs.x);
    std::swap(lhs.y, rhs.y);
}

int main() {
   Point a, b;
   a.x = 3;
   a.y = 4;
   swap(a, b);
}
```

## C-style-strings/ASCIIZ-строки/строки-в-стиле-Си

### c-str

Конвенция для хранения строк - массив байт `char arr[]`
Массив байт всегда(!) заканчивается нулевым символом, поэтому размер такого массива всегда (+1), тк мы можем завести указатель на строчку и вывести нашу строку (а ему нужно знать, куда ссылаться по памяти)


```C
char arr[] = "Hello";
//   char arr[] = {'H', 'e', 'l', 'l', 'o', 0 /*'\0'*/};
std::cout << sizeof(arr) << "\n";

char *str = arr;
std::cout << str << "\n";
```

Если не указывать `0` в конце - UB.

Тут работает арифметика указателей - тогда выведется суффикс строки

```C
char arr[] = "Hello";
char *str = arr + 2;
std::cout << str << "\n"; // llo
```

На самом деле тип строкового литерала (это не `char*` / `const char*`) - это `char [6]` массивы `char` размера - длина строки + 1
```C
std::cout << boost::core::demangle(typeid("Hello").name()) << "\n";
```

Но могут быть проблемы, если вставить символ `\0` в середину строки.
В случае, когда мы выводим всю строку - нулевой символ считается пробелом
```C
    char arr[] = "Wow\0Foo";
std::cout << sizeof(arr) << "\n";                                   // 8
std::cout << arr << "\n";                                           // Wow
std::cout << std::string(arr) << "\n";                              // Wow
std::cout << std::string(arr, arr + sizeof(arr)) << "\n";           // Wow Foo
std::cout << std::string(std::begin(arr), std::end(arr)) << "\n";   // Wow Foo
std::cout << arr + 4 << "\n";                                       // Foo
```

### (де)сериализация
Если хранится массив `char`, то можем быстро записывать в файл и читать из него эту информацию.
```C
struct Person {
    char first_name[31]{};
    char last_name[31]{};
};

int main() {
    {
        Person p;
        std::strcpy(p.first_name, "Konstantin");
        std::strcpy(p.first_name, "Ivan"); //Тут перезапишется, но будет остаток Константина^^
        std::strcpy(p.last_name, "Ivanov");
        {
            std::ofstream f("01.bin", std::ios::binary);
            f.write(reinterpret_cast<const char*>(&p), sizeof p);
        }
    }      
    {
        Person p;
        {
            std::ifstream f("01.bin", std::ios::binary);
            f.read(reinterpret_cast<char*>(&p), sizeof p);
        }
        std::cout << p.first_name << "\n";
        std::cout << p.last_name << "\n";
    }
}
```

Но с `char*` такое уже не прокатит. Тк просто запишутся указатели да память для этого конкретного файла. Аналогично будет и для `std::string`
```C
struct Person {
    char *first_name;
    char *last_name;
    /*
    std::string first_name;
    std::string last_name;
    */
};

int main() {
    Person p;
    p.first_name = "Ivan";
    p.last_name = "Ivanov";

    {
        std::ofstream f("02.bin", std::ios::binary);
        f.write(reinterpret_cast<const char*>(&p), sizeof p);
    }
    /*
        std::ofstream f("03.bin", std::ios::binary);
        f.write(reinterpret_cast<const char*>(&p), sizeof p);  // UB.
        // Потому что std::string - не POD. Точнее: не Trivial. Точнее: не TrivialCopyable.
        // Надо что-то такое (плюс как-то указать длину, чтобы можно было прочитать):
        f.write(p.first_name.data(), p.first_name.size());
    */
}
```

### Строковой литерал
Время жизни строкового литерала - `static`. За ним полностью следит компилятор, это константная память, её изменения запрещены. Поэтому указатели нужны константные.

#### std::strlen
```C
char arr[] = "Hello World";
std::cout << std::strlen(arr) << "\n";      // 11
std::cout << std::strlen(arr + 3) << "\n";  // 8

// реализация
auto my_strlen = [&](const char *s){
    int len = 0;
    while (s[len] != 0) {
      len++;
        }
    return len;
    };

std::cout << my_strlen(arr) << "\n";        // 11
std::cout << my_strlen(arr + 3) << "\n";    // 8
```


#### std::strcpy
иногда хочется суеты и написать свой `strcpy`, на заметку, он пишется вот так -

```C
void my_strcpy(char *dest, const char *src) {
    for (int i = 0;; i++) {
        dest[i] = src[i];
        if (!src[i]) {      //скопировали нулевой байт
            return;
        }
    }
}

void run(char *foo, char *bar, char *baz) {
    my_strcpy(foo, bar);  // foo = bar
    std::cout << "|" << foo << "|" << bar << "|\n"; // World World

    my_strcpy(foo, baz);
    std::cout << "|" << foo << "|\n";               // Hi
                                                    // если не было if(!src[i]), было бы 'Hirld'

    my_strcpy(foo, "12345");  // OK.                // 12345
    std::cout << "|" << foo << "|\n";

    my_strcpy(foo, "123456");  // UB! (выход за границы массива) Но мы про это не знаем.
    std::cout << "|" << foo << "|\n";

    std::strcpy(foo, "123456");  // UB! Но мы про это не знаем.
    std::cout << "|" << foo << "|\n";
}

int main() {
    char foo[] = "Hello";
    char bar[] = "World";
    char baz[] = "Hi";
    run(foo, bar, baz);
}

```

Как обычно лечат это UB? - с помощью `strncpy` (но все равно не безопасно, тк могут размеры поехать)

```C
void my_strncpy(char *dest, const char *src, int n) {
for (int i = 0; i < n; i++) {   // <n
    dest[i] = src[i];
    if (!src[i]) {
        return;
    }
}
}

int main() {
char foo[] = "Hello";
my_strncpy(foo, "123456", sizeof foo);  // UB отсутствует.
std::cout << foo << "\n";  // UB! Строчка не null-terminated.

strncpy(foo, "123456", sizeof foo);  // UB отсутствует.
std::cout << foo << "\n";  // UB! Строчка не null-terminated.
}
```

Прелесть в том, что можно поделить строку, вставив на проебельные символу 0, без перевыделения памяти (с `std::vector` такое не прокатит)
```C
   char command_line[] = "arg1 arg2 arg3";
// std::strtok занимается примерно этим.
for (int i = 0; command_line[i]; i++) {
    if (command_line[i] == ' ') {
        command_line[i] = 0;
    }
}

std::vector<char*> args = {
    command_line + 0,
    command_line + 5,
    command_line + 10,
};
std::cout << args[0] << "\n"; // arg1
std::cout << args[1] << "\n"; // arg2
std::cout << args[2] << "\n"; // arg3
```

`sizeof(argv[i])` - берет размер типа, то есть это равно `sizeof(char*`, так вообще делать нельзя, если строчка не `const`!, если нужен размер - `strlen`
```C
for (int i = 0; i < argc; i++) {
    std::cout << "argv[]="
              << argv[i]
              << "; strlen="
//                  << sizeof(argv[i])  // Так нельзя делать никогда! Это sizeof(char*)
              << std::strlen(argv[i])
              << "; p="
              << static_cast<void*>(argv[i]) << "\n";
}
```


#### Конкатенация
Так делать нельзя - складывать 2 указателя - бред
```C
/*
    char* a = "hello ";
    const char* b = "world";
    std::cout << (a + b) << "\n";
*/
    //но так можно
     std::cout << ("Hello" " World") << "\n";  // Костыль на этапе компиляции(!).
                                               // Только для литералов.

    // std::cout << (a b) << "\n";             // Так тоже нельзя

```

Время жизни строкового литерала - `static`.

замечание : `const char*` -> `char*` нельзя, но наоборот можно

вот такой свой конкатенатор вызывает UB, потому что никто не гарантировал нам, что в память после этой строки можно еще что-то записывать
```C
 // Расширение GCC: можно записывать строковой литерал в char* для совместимости со старым кодом.
    // Но писать туда всё ещё нельзя.
    /*const*/ char* a = "hello ";
```

```C
void my_strcat(char *a, const char *b) {
    int a_pos = std::strlen(a);
    for (int b_pos = 0; b[b_pos]; b_pos++) {
        std::cout << "b_pos=" << b_pos << "\n";
        a[a_pos++] = b[b_pos];
    }
    a[a_pos] = 0;
}
```

Литералы могут склеиваться
```C
const char *p1 = "hello";
const char *p2 = "hello";
//указатели совпадут
std::cout << static_cast<const void*>(p1) << " " << static_cast<const void*>(p2) << "\n";
```


```C
char arr[] = "hello "; //если тут большой буфер, то может быть и не UB
my_strcat(arr, b);  // UB: выход за границу массива.
std::cout << arr << "\n";
```

Важно заводить буфер с учетом нулевого литерала
```C
char arr[11] = "hello ";
const char* b = "world";
my_strcat(arr, b);  // UB: нет места под null-terminator. Стреляет редко, но больно.
std::cout << arr << "\n";
std::cout << strlen(arr) << "\n";
assert(strlen(arr) == 11);
```

Попробуем написать `my_strcat` лучше
```C
void my_strcat(char *&a, const char *b) {
char *out = new char[strlen(a) + strlen(b) + 1];

std::strcpy(out, a);
std::strcpy(out + strlen(a), b);

delete[] a;
a = out;
}
```

Такой код будет работать, но если создаем строку не через `new`, это работать не будет
```C
// ОК, но очень странно выглядит.
char *a = new char[6];
std::strcpy(a, "hello");
char b[] = " world";
my_strcat(a, b);
// my_strcat(a + 1, b);  // UB: будет delete из середины куска памяти.
std::cout << a << "\n";
delete[] a;
```

Это UB, тк тут удаляем часть массива
```C
char a[] = "hello";
char b[] = " world";
char *aptr = a;
my_strcat(aptr, b);
std::cout << aptr << "\n";
```

#### Вспомним GTA и функцию чтения из c-str

Хороший вариант написать функцию, которая будет работать на самом деле за `O(n^2)`

```C
auto read_int = [](const char *s) {
    int value = 0;
    for (std::size_t i = 0; i < std::strlen(s); i++) {
        if ('0' <= s[i] && s[i] <= '9') {
            value = value * 10 + s[i] - '0';
        } else {
            break;
        }
    }
    return value;
};

static char str[10'000'000];
for (std::size_t i = 0; i < sizeof(str); i++)
    str[i] = '0' + i % 10;
str[sizeof(str) - 1] = 0;

str[5] = ' ';
str[10] = ' ';
std::cout << read_int(str) << "\n";
for (int i = 0; i < 100'000; i++) {
    assert(read_int(str) == 1234);
}
```
### Про `std::string`
Это обёртка над сишными строками, которые умеют больше и сами следят за строкой, её размером и тд. Но можно попросить у `std::string` и сишную строку.
* `std::string::c_str()` - вернёт копию строчки.

* `std::string::data()` - вернёт константный указатель на начало строки.

## `const_cast`

`const_cast` - нужно в основном для совместимости со старым сишным кодом, в котором `const` не писали, поэтому если хочется передать константную
строку куда-то в Си, то нужно явно отбросить константность(небезопасно
и страшно).
Другая ситуация в которой используется `const_cast`. Не хотим реализовывать все get’ы, так как они различаются только
типом возвращаемой ссылки. Тогда реализуем один get, а остальные выразим через `const_cast`. Тонкость: можно отбрасывать константность только если мы ее сами добавили(иначе UB). Однако, в первой ситуации(отбрасываем
ради совместимости с Си) можно отбросить, так как просто читаем, но не
вызываем на объекте никакие методы

```C++
#include <stdio.h>
#include <cassert>
#include <utility>
void old_c_print_line(char *str) {
    printf("%s\n", str);
}
template <typename T>
class optional {
    T data{};
    bool initialized = true;
public:
    T &get() & {
        return const_cast<T &>(std::as_const(*this).get());
    }
    T &&get() && {
        return std::move(get());
    }
    const T &get() const & {
        assert(initialized);
        return data;
    }
    // Incorrect implementation:
    /*
    T& get() {
        assert(initialized);
        return data;
    }
    const T &get() const {
        return const_cast<optional*>(this)->get();
    }
    */
};
int main() {
    old_c_print_line(const_cast<char *>("hello"));
    optional<int> a;
    const optional<int> b;
    a.get() = 10;
    printf("%d\n", a.get());
    printf("%d\n", b.get());  // Should not const_cast inside! UB otherwise.
}
```

## Отличия-от-языка-Си

### Отличия С и C++
- Первый стандарт С89: переменные только в начале функции, неявные include (GCC всё ещё поддерживает, хотя убрали в C99), комментарии только /* wtf */
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
- Нет в том числе и anonymous namespace. Вместо него можно использовать ключевое слово `static` при объявлении функций или переменныхх. Тогда они не будут видны в других единицах трансляции.
```C
static void foo(void) {
}
void bar(void) { // видно в другом файле
}
static int x;
int y; // видно в другом файле
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

- преобразования с `void` для обобщения типов, тк нет шаблонов. В примере пердаются просто два указателя, а чтобы иетрироваться пердаём ещё размер элемента и их кол-во.
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
- Можно в качестве аргемента в функции передавать указатели на функцию (почти функтор). В данном примере так передаётся компаратор для сортировки.
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

## Языковые-особенности-Си

### `Union`
Если же вспоминать историю, union позволяет переиспользовать одну и ту же область памяти для хранения разных полей данных. Когда объявлено объединение, компилятор автоматически создает переменную достаточного размера для хранения наибольшей переменной, присутствующей в `union`.



#### C++:
**используйте `std::variant`**

Обращение тут к неправильному полю это сразу UB (accessing to non-active union member), но GCC конкретно в этом месте все равно.

```C++
#include <variant>

struct MouseEvent {
    unsigned x = 0;
    unsigned y = 0;
};

struct KeyboardEvent {
    unsigned scancode = 0;
    unsigned virtualKey = 0;
};

using Event = std::variant<
    MouseEvent,
    KeyboardEvent>;
```



#### Cи:

Обращение тут к неправильному полю это НЕ UB. Обычно компиляторы берут байты из памяти и прочитали их как другое поле (а вообще читайте документацию). _В примере ниже так как это int у нас гарантированно выведется `11`_

```C
#include <stdio.h>

enum EventType { MOUSE, KEYBOARD };

struct MouseEvent {  // 12
    int x;
    int y;
    int button;
};

struct KeyboardEvent {  // 8
    int key;
    int is_down;
};

union AnyEvent {  // 12
    struct MouseEvent mouse;
    struct KeyboardEvent keyboard;
};

// tagged union: std::variant<MouseEvent, KeyboardEvent>
// Haskell: tagged union тоже есть, называется "тип-сумма", и там синтаксис
// хороший и безопасный.
struct Event {  // 16
    int type;
    union AnyEvent event;
};

int main(void) {
    printf("%d\n", sizeof(struct Event));
    struct Event ev;
    ev.type = MOUSE;
    ev.event.mouse.x = 11;
    ev.event.mouse.y = 22;
    ev.event.mouse.button = 1;
    // https://stackoverflow.com/a/25672839/767632
    // Implementation-defined in C (object representation).
    // UB in C++ (accessing to non-active union member).
    // OK in GCC in C++:
    // https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#Type-punning
    printf("%d\n", ev.event.keyboard.key);  // UB in C++, 11 in C.
}
```


#### Анонимный union или struct

Union выше не помещается в одно окно. Давайте писать короче, нам же его имя не нужно. Если у него нет ни названия типа, ни названия поля, то просто добавятся поля анонимной структурки в нашу.

```C
#include <stdio.h>

enum EventType { MOUSE, KEYBOARD };

// tagged union: std::variant<MouseEvent, KeyboardEvent>
// Standard C and C++
struct Event {  // 16
    int type;
    union {  // Anonymous union, стандартный C и C++
        struct {
            int x;
            int y;
            int button;
        } mouse;
        struct {
            int key;
            int is_down;
        } keyboard;
    } /* event */;
};

int main(void) {
    printf("%d\n", sizeof(struct Event));
    struct Event ev;
    ev.type = MOUSE;
    ev /*.event*/.mouse.x = 11;
    ev /*.event*/.mouse.y = 22;
    ev /*.event*/.mouse.button = 1;
}
```

### Designator

Синтаксис для инициализации конкретных полей у структуры. И даже макросом!

```C
#include <stdio.h>

struct point {
    int x, y, z;
};

#define INIT_ARR [0] = 55, [1] = 56

int main(void) {
    struct point p = {.x = 10};
    // https://docs.python.org/3/extending/newtypes_tutorial.html#defining-new-types)

    int arr[10] = {INIT_ARR, [3] = 5, [5] = 10};
    for (int i = 0; i < 10; i++) {
        printf("arr[%d]=%d\n", i, arr[i]);
    }
}
```
А еще с макросами нужно быть аккуратнее, так как может случиться коллизия. Чтобы этого избежать, они принимают параметры, с какими объявить переменные.


### Restrict

Пусть мы хотим скопировать память откуда-то куда-то. Что будет при пересечении памяти? Тут много идеологических вопросов.

`memcpy` просто запрещает памяти пересекаться, если же пересеклись, то UB. Это сделано для того, чтобы в хлам оптимизировать.

`memmove` работает по-другому. Она разрешает памяти пересекаться, она не такая быстрая, но там стоит `if`, что если один указатель левее копируй в одну сторону, если другой, то в другую. И она корректно разбирает пересечения

`restrict`  - если написать это слово, то мы подсказываем оптимизатору, что указатели не пересекаются ОПТИМИЗИРУЙ (если конкретнее, то указатель не пересекаются с другими объектами, которые тут видны).

```C
#include <stdio.h>
#include <stdlib.h>

void my_memcpy1(void *restrict dst, const void *restrict src, size_t n) {
    // restrict: dst/src do not intersect and do not point to &n.
    while (n-- > 0)
        *(char *)dst++ = *(const char *)src++;
}

void my_memcpy2(void *restrict dst, const void *restrict src, size_t n) {
    for (size_t i = n; i > 0; i--) {
        ((char *)dst)[i - 1] = ((const char *)src)[i - 1];
    }
}

int main(void) {
    {
        int arr[] = {1, 2, 3, 4, 5};
        my_memcpy1(arr, arr + 2, sizeof(int) * 3);
        for (int i = 0; i < 5; i++) {
            printf("%d%c", arr[i], "\n "[i + 1 < 5]);
        }
    }
    {
        int arr[] = {1, 2, 3, 4, 5};
        my_memcpy2(arr + 2, arr, sizeof(int) * 3);
        for (int i = 0; i < 5; i++) {
            printf("%d%c", arr[i], "\n "[i + 1 < 5]);
        }
    }
    // memcpy: no intersections
    // memmove: allows intersections
}
```

## Идиомы-языка-Си

### Макросы

#### C++:

Не используйте макросы, используйте функции, компилятор всё сделает `inline`.
> **inline** - используется для запроса, чтобы компилятор рассматривал вашу функцию как встроенную. При компиляции вашего кода, все inline functions заменяются копией содержимого самой функции, и ресурсы, которые могли бы быть потрачены на вызов этой функции, сохраняются! Минусом является лишь увеличение компилируемого кода за счет того, что встроенная функция раскрывается в коде при каждом вызове.


#### Си:

Слово `inline` появилось поздно. Мы хотим все от железа, поэтому если есть часто вызывающаяся функция - пишем макрос.
Это работает быстрее, так как старые оптимизаторы не умеют оптимизировать вызовы функций, а хочется, чтобы запускалось и на старых.
```C
#define max(a, b) ((a) < (b) ? (b) : (a))
// Inline functions are since C99 only, not everyone supports them.

#define json_object_object_foreach(obj, key, val)                          \
    char *key = NULL;                                                      \
    struct json_object *val = NULL;                                        \
    struct lh_entry *entry##key;                                           \
    struct lh_entry *entry_next##key = NULL;                               \
    for (entry##key = json_object_get_object(obj)->head;                   \
         (entry##key ? (key = (char *)lh_entry_k(entry##key),              \
                       val = (struct json_object *)lh_entry_v(entry##key), \
                       entry_next##key = entry##key->next, entry##key)     \
                     : 0);                                                 \
         entry##key = entry_next##key)
// `json_object_object_foreach` is harder to optimize because it would need
// pointer-to-function optimization.
```

### Go to

На самом деле тут зашифрован цикл. Попробуйте это понять! Да, сложно. Поэтому Дейкстра и сказал, что не надо так больно, пойдемте писать в фигурных скобках.
```C
#include <stdio.h>

// Go To Statement Considered Harmful:
// https://homepages.cwi.nl/~storm/teaching/reader/Dijkstra68.pdf

int main(void) {
    int n = 10;
    int i = 0;
label:
    printf("%d\n", i);
    i++;
    int x = 5;
    printf("  x=%d\n", x);
    x++;
    if (i < n)
        goto label;
    printf("end\n");

    {
    foo:;
    }
    {
    bar:;  // Not in C++?
        int x;
    }
}
```

**Тонкости:**

* Метка всегда относится к какому-то стейтменту (поэтому если делаем метку перед `}`, то ставим `;`, а еще не забываем, что объявление переменной это не стейтмент, поэтому тоже надо добавить `;`).

* Go to может прыгать и вперед и назад

В примере ниже зашифрован `if

```C
#include <stdio.h>

int main(void) {
    int n = 10;
    if (n >= 5)
        goto big_n;
    int c = 100;
    printf("n is small\n");
    n += 100;
big_n:
    printf("n is big: %d; c=%d\n", n, c);
}
```

А тут мы прирываем циклы

```C
#include <cstdio>
#include <utility>

int main() {
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++) {
            if (i * j == 24) {
                std::printf("%d %d\n", i, j);
                goto after_loop;
            }
        }
    after_loop:;

    auto [a, b] = []() {
        for (int i = 0; i < 10; i++)
            for (int j = 0; j < 10; j++) {
                if (i * j == 24) {
                    return std::make_pair(i, j);
                }
            }
    }();
    std::printf("%d %d\n", a, b);
}
```

* Языку Си плевать на неинициализированную переменную, потому что `goto` использовали раньше очень часто. Тем не менее тут чтение из переменной `c` Это UB. По этой причине нормальный `if` лучше - он делает больше проверок. С++ за такое банят в принципе.

Goto  очень часто используется для **обработки ошибок**.

_Пусть у нас есть файлик и мы в нем сначала считываем `n`, потом `n` чисел. Мы хотим это считать в память и вывести в обратном порядке._
  * _Открыть файл_
  * _Считать число `n`_
  * _Выделить массив размера `n`_
  * _Считываем `n` чисел_
  * _Выводим массив в обратном порядке_

```C
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    int ret = 1;

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Unable to open file\n");
        goto f_closed;
    }

    int n;
    if (fscanf(f, "%d", &n) != 1) {
        printf("Unable to read n\n");
        goto f_opened;
    }
    int *arr = malloc(n * sizeof(int));
    if (!arr) {
        printf("Unable to allocate array for %d ints\n", n);
        goto buf_freed;
    }

    for (int i = 0; i < n; i++) {
        if (fscanf(f, "%d", &arr[i]) != 1) {
            printf("Unable to read element %d\n", i + 1);
            goto err;
        }
    }

    // WONTFIX: check printf result. No assert() because it can be removed.
    printf("Result: ");
    for (int i = n - 1; i >= 0; i--) {
        printf("%d", arr[i]);
        if (i > 0)
            printf(" ");
    }
    printf("\n");
    ret = 0;

err:
    free(arr);
buf_freed:
f_opened:
    fclose(f);
f_closed:;

    return ret;
}
```

Так у нас нет дублирования кода при освобождении ресурсов. У нас просто правильно расставлены метки, начинающие закрывать ресурсы, начиная с какого-то.

В плюсах это все делает благодаря деструкторам.

### Scanf

**scanf** - ввода общего назначения, считывающей данные из пото­ка stdin. Она может считывать данные всех базовых типов и автоматически конвертировать их в нужный внутренний формат.

>* %с	Считать один символ
* %d	Считать десятичное число целого типа
* %i	Считать десятичное число целого типа
* %е	Считать число с плавающей запятой
* %f	Считать число с плавающей запятой
* %g	Считать число с плавающей запятой
* %о	Считать восьмеричное число
* %s	Считать строку
* %х	Считать шестнадцатиричное число
* %р	Считать указатель
* %n	Принимает целое значение, равное количеству считанных до текущего момента символов
* %u	Считывает беззнаковое целое
* %[]	Просматривает набор символов
* %%	Считывает символ %

```C
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // https://en.cppreference.com/w/c/io/fscanf
    char buf1[10], buf2[20];
    int x;
    int read1 = scanf(" %9[^-]-%19s%*d%d", buf1, buf2, &x);
    printf("read1=%d\n", read1);

    float a;
    double b;
    int read2 = scanf("%f%lf", &a, &b);  // types should match!
    printf("read2=%d\n", read2);

    // https://en.cppreference.com/w/c/io/fprintf
    printf("buf1=|%s|\nbuf2=|%s|\nx=%d\n", buf1, buf2, x);
    printf("     01234567890123456789\nbuf1=%9s\nbuf2=%19s\nx=%05d\n", buf1,
           buf2, x);
    printf("\n% d\n% d\n% d\n", 0, 5, -5);
    printf("\n%+d\n%+d\n%+d\n", 0, 5, -5);
    printf("%010.3f\n", a);
    printf("%010.3f\n", b);  // not %lf! ellipsis conversions.
    printf("100%% done!\n");
}
```
Разберем  строчку ниже на вводе `hi wor-    meow-meow 4 5`

```C
scanf(" %9[^-]-%19s%*d%d", buf1, buf2, &x)
```
Сначала пропустим произвольное количество пробельных символов. Потом он будет считывать максимум 9 любых символов не включая `-`.
Дальше считает `-`. После строка максимального размера 19 до пробела с пропуском пробельных символов. Звездочка говорит считать, но не записывать тоже с пропуском пробельных символов.

Итого:

```
read1=3
buf1=|hi wor|
buf2=|meow-meow|
x=5
```

Можно читать вещественные. Типы должны совпадать потому что программа ожидает определенные байты в определенном формате.

#### Scanf_s

Это есть в стандарте, но поддерживает только Вижак.

```C
#include <stdio.h>

int main() {
    {
        char buf[5];
        int cnt = scanf("%s", buf);  // Potentially UB
        printf("cnt=%d, buf=|%s|\n", cnt, buf);
    }
    {
        char buf[5];
        int x;
        int cnt = scanf("%4s%d", buf, &x);  // No UB, will stop reading after 4 bytes.
        printf("cnt=%d, buf=|%s|, x=%d\n", cnt, buf, x);
    }
#ifdef __WIN32__  // It's in C11, but is not supported by libstdc++/libc++
    {
        char buf[5];
        int cnt = scanf_s("%s", buf, (rsize_t)sizeof buf);  // No UB, will call "constraint handler" (implementation-defined) and fail reading.
        printf("cnt=%d, buf=|%s|\n", cnt, buf);
    }
#endif
}
```

#### Sscanf

**Sscanf** - аналогично scanf читает из строки. И возвращает текущую позицию в строке. (То есть на сколько сдвинули указатель и где кончили читать). Работает за линию от ВВОДА.

```C
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    char buf[] = " 123+   45+8 79+4";
    int pos = 0;
    while (buf[pos]) {
        if (buf[pos] == '+') {
            pos++;
            continue;
        }
        int x, read;
        // May take linear time! Like in GTA: https://nee.lv/2021/02/28/How-I-cut-GTA-Online-loading-times-by-70/
        int res = sscanf(buf + pos, "%d%n", &x, &read);
        assert(res == 1);
        printf("pos=%d; read=%d; x=%d\n", pos, read, x);
        pos += read;
    }
}
```
Посмотрим на иерархию:
```Text
fscanf
scanf --> fscanf(stdin, ...)
sscanf --> создай файл + fscanf
создай файл --> создай структуру {указатель на строчку, длина строки ЗА O(n)}
```

### Printf

**Printf** -  записывает в stdout аргументы из списка arg-list под управлением строки, на которую указывает аргумент format.

> Вормат такой же как в scanf

```C
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // https://en.cppreference.com/w/c/io/fscanf
    char buf1[10], buf2[20];
    int x;
    int read1 = scanf(" %9[^-]-%19s%*d%d", buf1, buf2, &x);
    printf("read1=%d\n", read1);

    float a;
    double b;
    int read2 = scanf("%f%lf", &a, &b);  // types should match!
    printf("read2=%d\n", read2);

    // https://en.cppreference.com/w/c/io/fprintf
    printf("buf1=|%s|\nbuf2=|%s|\nx=%d\n", buf1, buf2, x);
    printf("     01234567890123456789\nbuf1=%9s\nbuf2=%19s\nx=%05d\n", buf1,
           buf2, x);
    printf("\n% d\n% d\n% d\n", 0, 5, -5);
    printf("\n%+d\n%+d\n%+d\n", 0, 5, -5);
    printf("%010.3f\n", a);
    printf("%010.3f\n", b);  // not %lf! ellipsis conversions.
    printf("100%% done!\n");
}
```
Можно указать ширину (для выравнивания пробелами).
Можно выравнивать не пробелами, а нулями.
Можно выводить знак.
Можно выводить ровно n знаков до запятой и добивать начало нулями.

**Ellipsis conversions** - параметры приводятся к базовым типам.

#### Sprintf

**Sprintf** - выводит переменную в буфер и никак не проверяет переполнился ли буфер

#### Snprintf

**Snprintf** - выводит переменную в буфер и проверяет переполнился ли буфер так как мы передаем в него размер. Вернет он количество символов, которые ему БЫ ХОТЕЛОСЬ записать в буффер. (Чтобы мы могли узнать, что нам чего-то не хватило, сделать буфер побольше и сделать запись)

```C
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    char buf[10];
    int pos = 0, out;

    out = snprintf(buf, sizeof buf, "%d+", 10);
    printf("out=%d\n", out);
    assert(out >= 0);
    pos += out;

    out = snprintf(buf + pos, sizeof buf - pos, "%d", 12345);
    printf("out=%d\n", out);
    assert(out >= 0);
    pos += out;

    out = snprintf(buf + pos, sizeof buf - pos, "+%d", 426);
    printf("out=%d\n", out);
    assert(out >= 0);
    pos += out;

    // OOPS: UB is close by because of (buf + pos)

    printf("buf=|%s|, pos=%d\n", buf, pos);
}
```
Тут мы записываем при помощи нескольких `snprintf` и каждый раз дописываем в конец буфера.

Такой вывод у кода выше:
```
out=3
out=5
out=4
buf=|10+12345+|, pos=12
```
По-хорошему нужно передавать минимум. В общем возможность безопасно написать есть, но ыыыы

#### Gets

`gets` считывает строчку из стандартного потока ввода, при ввыводе `%s` - значит тут должна быть строка
```C
std::printf("> ");
char command[10];   //команда максимум из 9 символов
gets(command);  // Может случиться переполнение буфера (buffer overflow).
// Top-1 уязвимостей в мире: https://ulearn.me/course/hackerdom/Perepolnenie_steka_3bda1c2c-c2a1-4fb0-9146-fccc47daf93b

std::printf("command is |%s|\n", command);
```

Как это чинить? (ограничение буффера) - `fgets` (но строка обрежеься, если она будет больше, чем мы указали)
```C
std::printf("> ");
char command[10];
std::fgets(command, sizeof command, stdin);  // Всегда делает null-terminated строчку.
                                             // Обрезает ввод.
// Плюсовый std::string не обрезает, но может слопать всю память :(
std::printf("command is |%s|\n", command);
```

### Realloc/Malloc/Free

**Malloc** - функция, позволяющая динамически выделять память (удобно, когда нам не известем заранее размер нужного пространства). Очень важно эту память ручками потом за собой почистить при помощи `free`.

**Realloc**  - передаем указатель и новый размер. Функция проверяет можно ли расширить буфер прямо тут. Если можно - расширяем, если нет - выделяем новую память, копируем  в него, а старое освобождаем. Поэтому мы иногда вообще не тратим ресурсы на копирование.

```C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char *s = malloc(5), *new_s;
    strcpy(s, "xyz");
    printf("%p %s\n", s, s);

    s = realloc(s, 4);
    printf("%p %s\n", s, s);

    // FIXME: what is the problem?
    // https://pvs-studio.com/en/w/v701/
    // https://habr.com/ru/company/pvs-studio/blog/343508/
    s = realloc(s, 6);
    printf("%p %s\n", s, s);

    s = realloc(s, 100);
    printf("%p %s\n", s, s);

    s = realloc(s, 1000);
    printf("%p %s\n", s, s);

    free(s);
}
```

**Про проблему:** если у нас не получилось выделить память, то в С++ кидается исключение, а в Си `NULL`. Поэтому на самом деле нужно везде ставить проверки на то, не вернули ли мы `NULL`. Но если мы записываем что-то новое в себя, то старое еще не освободилось, а указатель не сохранен. Поэтому правильнее реализовывать по-другому.

```C
new_s s = realloc(s, 6);
if(new_s == NULL){
  free(s);
}
s = new_s;
printf("%p %s\n", s, s);
```

**Заметки:**

`malloc` в С++ это UB, потому что мы выделили память но не создали в ней объект (не вызвали конструктор). Но линковать с С++ даже нормально.



## Линковка-программ-на-Си/C++

### Совместная линковка

Сразу покажем правильный вариант:

| a.c                                      |  b.cpp                     |
| ---------------------------------------- | :-------------------------:|

```Rust
#include <cstdio>                              #include <stdio.h>
#include <iostream>                            int my_main(void);
#include <vector>                              int foo() {
                                                  int arr[] = { [3] = 123; };  
extern "C" int foo();                             // Use C-specific syntax to
// Disable name mangling, and link like C.        // ensure we're writing C.
                                                  return arr[3];
int foo(int x) {  // OK                        }
    return x + 1;
}
                                              int main(void) {
namespace a {                                     printf("hello\n");
#if 0                                             return my_main();
// Warning: same as foo(),                    }
// but different set of arguments.
// It's ok in C, though (e.g. printf).
extern "C" int foo(int);
#endif
}  // namespace a

struct Foo {
    Foo() {
        std::printf("Foo\n");
    }
    ~Foo() {
        std::printf("~Foo\n");
    }
} f;

extern "C" int my_main() {
    std::vector<int> v;
    std::cout << foo() << "\n";
    std::cout << foo(100) << "\n";
    return 0;
}
```


В Си нет перегрузок, поэтому функции можно называть своими собственным именами. А в C++ перегрузки есть, поэтому в С++ каждая функция перед тем, как перейти в объектный файл, проходит через **mangling** (изменение имени). Поэтому в языке C++ нужно пометить, что данная функция  будет линковаться с языком Си и назвать ее нужно по правилам языка Си. Из этого следует то, что нам все-таки можно иметь перегрузку, потому что компилятор поймет вызывать простую или плюсовую, а в `namespace` это делать нельзя.

```
gcc a.c -c        # gcc or `-x c`, otherwise will assume c++ regardless of extension
gcc b.cpp -c      # gcc or g++ both work(?)
```
Если линковать при помощи GCC , то будет упс.
В С++ есть стандартная библиотека и нам нужны реализации этих функций. G++ их по умолчанию добавляет, GCC нет.

```
g++ a.c b.cpp  # g++ considers all files to be C++ regardless of extension
```
 Можно слинковать явно, но лучше использовать GCC.
 ```
g++ a.o b.o -o a  # g++! Should include C++ standard library: https://stackoverflow.com/a/172592/767632
gcc a.o b.o -lstdc++ -o a
 ```

### Один заголовок на два языка

В Си нет `extern "C"`, поэтому чтобы сделать один заголовок на два языка и в C++ `extern "C"` дописывалось, а в Си нет, делаем вот так
 ```C
 #ifdef __cplusplus
extern "C" {
#endif

int foo(void);  // Remember to say "no arguments" in C! It's compatible with C++
int my_main();

#ifdef __cplusplus
}
#endif
 ```
