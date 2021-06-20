# 5x Метапрограммирование

1. [constexpr, функции на этапе компиляции](#constexpr, функции на этапе компиляции)
1. [Функции из типов в типы](#Функции из типов в типы)
1. [type-traits](#type-traits)
1. [оператор noexcept](#оператор noexcept)
1. [type display trick](#type display trick)
1. [std::addressof](#std::addressof)

## constexpr, функции на этапе компиляции
Хотим научиться делать вычисления на стадии компиляции.
Например знаем, что факториал некоторой константы, значение которой известно на стадии компиляции
тоже можно вычислить на стадии компиляции. Хотели бы написать так: 

```C++
int factorial(int n) {
    int res = 1;
    for (int i = 1; i <= n; ++i) {
        res *= i;
    }
    return res;
}
constexpr factorial5 = factorial(5); // не ок, так как нельзя вычислить factorial в compile-time
std::array<int, factorial(5)> //опять не ок, параметры шаблона всегда константы во время компиляции
```

Но к сожалению, хоть и кажется, что функцию `factorial` можно вычислить в compile-time,
компилятор про это не в курсе, поэтому надо специальным образом пометить функцию `factorial` словом `constepxr`. То есть такой код уже будет работать:
```C++
constexpr int factorial(int n) {
    int res = 1;
    for (int i = 1; i <= n; ++i) {
        res *= i;
    }
    return res;
}
constexpr factorial5 = factorial(5); //ok
std::array<int, factorial(5)> //ok
```
Таким образом, с помощью слова `consexpr` можно помечать функции и переменные.
Ограничения на `constexpr` функции:
* C++11
  * Можно использовать только `return`, все пишется рекурсивно.
* C++14
  * Можно писать циклы и заводить переменные. Также можно изменять переменные.
  * Можно использовать примитвыне типы и типы с `constexpr`-конструктором и примитивным деструктором.
* До C++20
  * Нельзя использовать динамическое выделение памяти (как следствие нельзя использовать `std::vector`)
  
  Если из `constexpr`-функции выкинуть исключение, то здесь возможно 2 варианта:
  * Исключение было выкинуто на стадии компиляции (функцию вызвали на стадии компиляции), тогда просто произойдет ошибка компиляции.
  * Функцию вызвали в runtime -- исключение ведет себя обычным образом.

## Функции из типов в типы
Хотим получать соответствующий данному типу тип. Например, по итератору получать тип, который он внутри себя хранит. Возможная реализация:

```C++
template <typename T>
struct iterator_traits {
	using value_type = typename T::value_type; // Обязательно использовать typename, чтобы компилятор был уверен, что T::value_type это тип.
	using difference_type = typename T::difference_type;
};

template <typename T>
struct iterator_traits<T*> {
	using value_type = T;
	using difference_type = std::ssize_t;
}; //специализация для частного случая.
```

Теперь можно использовать так:
```C++
iterator_traits<std::vector<int>::iterator>::value_type x = 10;
```
Как-то очень громоздко, чтобы это сокращать (а именно не писать `::value_type`) можно писать шаблонные псевдонимы. Их принято писать с суффиксом _t (если мы по типу получаем другой тип) или с суффиксом _v (если мы по типу получаем значение).
```C++
template<typename T>
using iterator_traits_t = typename iterator_traits<std::vector<int>::iterator>::value_type;
iterator_traits_t<std::vector<int>::iterator> x = 10;
```

Вот пример когда мы хотим узнать количество размерностей в типе: (например 
`rank_v<int[][][]> = 3`)

```C++
template <typename T>
struct rank {
	static constexpr std::size_t value = 0;
};

template <typename T>
struct rank<T[]> {
	static constexpr std::size_t value = rank<T>::value + 1;
};

template <typename T, std::size_t N>
struct rank<T[N]> {
	static constexpr std::size_t value = rank<T>::value + 1;
}; //  с точки зрения языка T[] и T[N] разные типы

template <typename T>
constexpr std::size_t rank_v = rank<T>::value;
```

Стоит задуматься, что в таких конструкциях очень много общего, почти всегда есть `value` и смысл почти не меняется. В связи с этими целями давайте обобщать метапрограммирование 
с помощью наследования :) Вот пример структурки от которой мы будем наследоваться, у которой есть все что нужно:

```C++
template <typename T, T value_>
struct integral_constant {
	static constexpr T value = value_;
	using value_type = T;
	constexpr operator T() {
		return value_;
	} //оператор каста к типу T, чтобы ::value постоянно не писать
};
```

Тогда `rank` будет выглядеть вот так:

```C++
template <typename T>
struct rank : integral_constant<std::size_t, 0> {};

template <typename T>
constexpr std::size_t rank_v = rank<T>::value;

template <typename T>
struct rank<T[]> : integral_constant<std::size_t, rank_v<T> + 1>{};

template <typename T, std::size_t N>
struct rank<T[N]> : integral_constant<std::size_t, rank_v<T> + 1>{};
```
Можно заметить, что мы сильно уменьшили код. Учитывая, что `integral_constant` подходит под большое количество задач, это достаточно полезно. Сейчас рассмотрим еще одно применение. Вспомним задачку про факториал:

```C++
template <typename T, T N> fac{...};
template <typename T> fac<T, 0>{...};
```

И выесняется, что по каким-то странным причинам так специализировать нельзя (то есть только по значению). Решение проблемы: давайте значение запихнем в тип, а именно в `integral_constant`. Теперь вот так все будет работать:

```C++
template <typename > fac{};
template <typename T, T N> 
constexpr T fac_v = fac<std::integral_constant<T, N>>::value;

template <typename T, T N>
struct fac<std::integral_constant<T, N>> : std::integral_constant<T, N * fac_v<T, N - 1>> {};

template <typename T>
struct fac<std::integral_constant<T, 0>> : std::integral_constant<T, 1> {};

static_assert(fac_v<5> == 120);
```

Очень частый случай когда `T = bool` и его принято выделять (в целях читабельности и сокращения кода):

```C++
template <bool value>
struct bool_constant : std::integral_constant<bool, value> {};
using true_type = bool_constant<true>;
using false_type = bool_constant<false>;
```

В дальнейшем еще будем это использовать.

## type-traits

Если какой-то класс хранит внутри себя некоторую информацию о типе, то такая штука обычно называется типаж (type-traits).

```C++
template<typename T, typename U>
struct is_same : false_type {};

template<typename T>
struct is_same<T, T> : true_type {};

template <typename T, typename U>
static constexpr bool is_same_v = is_same<T, U>::value; 
```

Как видим, `is_same` внутри себя хранит информацию равны ли типы `U` и `T`. Также в стандартной библиотеке реализованы всякие type-traits по типу `std::remove_const` или `std::remove_reference`.

Еще есть сложные type-traits, которые требуют дополнительные инструменты компилятора, например `std::is_polymorphic`, который проверяет, что в классе присутствует виртуальная функция (своя или отнаследованная).

Про расширяемы type-traits, например хотим специализировать свой собственный iterator для своего класса, чтобы работали всякие штуки по типу std::distance. Например специализация может пригодиться, если внутри нашего итератора нет `value_type`.

```C++
template <typename T>
struct iterator_traits <MyMagicIterator<T>> {
	using value_type = typename MyMagicIterator<T>::magic_value_type;
	//....
};
```
Еще есть `std::char_traits<T>`, которые позволяют работать со всякими символьными типами.
Например если мы хотим сделать свой класс, который сравнивает 2 символа вне зависимости от регистра, то мы получим что-то такое:

```C++

struct case_insensitive_traits : char_traits<char> {
	static bool lt(char a, char b) {tolower(a) < tolower(b)};
	static bool eq(char a, char b) {tolower(a) == tolower(b)};
	//...
}; 

using case_insensitive_string = std::basic_string<char, case_insensitive_traits>;
```

Обычно свои `char_traits` это ужас и `tolower` -- древняя штука из C, ее тоже не используйте.

С этим нужно быть осторожным. Если вы взяли чей-то хэдер, специализировали свой класс (в своем хэдере), то потом можно случайно получить нарушение ODR. Более подробно:
В `library.h` есть класс `Pupa<T>`. Что сделали вы в своем хэдере `my.h`:

```C++
//include-guards
#include "library.h"

template<>
struct Pupa<int> {
//...
};
```

Теперь рассмортим два разных TU:

```C++
//include-guards
#include "my.h"

Pupa<int> kek;
```

```C++
//include-guards
#include "library.h"

Pupa<int> lol;
```

По итогу: в первом TU `Pupa<int>` проинстанцировался как ваша специализация, а во втором как задумывал автор библиотки (то есть по другому). И вот у вас появились разные реализации одного и того же класса. Поздравляю, вы нарушили ODR!
Вывод: везде используйте  оба хэдера.

## оператор noexcept

Этот оператор позволяет по выражению (не вычисляя его) проверить, что оно не кидает исключений (а это зависит от спецификаторов `noexcept`).
Пример:

```C++
const int a = 11;
static_assert(noexcept(a == 10));
static_assert(!noexcept(new int{})); //утечки не будет, сам код не выполняется.
```

Вспомним про спецификатор `noexcept`  у функций. Его можно сделать условным (то есть в скобочки записать значение, если оно `true`, то функция будет помечена noexcept, иначе нет):

```C++
void foo() noexcpet(true) {}
void foo() noexcpet {} //асболютно одинаковые записи
```

А еще можно использовать оператор внутри спецификатора:

```C++
template <typename T>
void foo() noexcept(noexcept(T())) {
//...
}
```

Теперь функция будет помечена noexcept тогда и только тогда, когда у типа `T` конструктор по умлочанию noexcept. Еще существует всякие type-traits по типу `std::is_nothrow_move_constructible`.

Допустим хотим в noexcept() проверить, что оператор `+` не кидает исключений. Делали бы так:
```C++
template <typename T>
void foo() noexcept(noexcept(T() + T())) {
//...
}
```
Только вот T необязательно констурируется по умолчанию и мы не хотим проверять, что конструктор по умолчанию noexcept. Поэтому есть `std::declval<T>()`, который можно использовать только внутри невычислимых контекстов:

```C++
template <typename T>
void foo() noexcept(noexcept(std::declval<T>() + std::declval<T>())) {
//...
}
```

Еще его можно и внутри `decltype` или `sizeof` использовать -- оба невычислимые контексты.

```C++
decltype(std::declval<long long>() + std::declval<int>());
sizeof(std::declval<long long>() + std::declval<int>());
```

## type display trick
Когда метапрограммируем, обычно хочется узнавать типы переменных (например для отладки) со всеми ссылками и константностью. Тогда можно сделать вот такой трюк и узнать тип переменной по CE:

```C++
template <typename T> stuct TD; // только forward-declaration

TD<decltype(foo())>{}; //будет подробная ошибка
```

## std::addressof
Для справки: оператор `&a` (взятия адреса) именно оператор, то есть его можно перегрузить. А когда вы создаете библиотеку, то должны создавать код, который работает во всех случаях (даже когда пользователь не очень умный и зачем-то перегрузил этот оператор).  Для этого есть стандартный `std::addressof`. Предпологаемая упрощенная реализация такая:

```C++
template <typename T>
T *addressof(T &value) {
	return reinterpret_cast<T *>(&reinterpret_cast<char&>(value)); //Все таки у char нельзя перегрузить этот оператор.
}
```
Еще нужно сделать несколько перегрузок для константых случаев, но тут этого нет.


