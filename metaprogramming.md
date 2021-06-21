# Метапрограммирование #

## Небольшое введение ##

Что такое метапрограммирование?
В общих словах, это когда вы пытаетесь заставить комиплятор написать программу за вас. Например, шаблоны- вполне себе метапрограммирование.

## Template parameter pack и variadic template ##

Идея- можем передавать в шаблон несколько разных параметров.
Начиная с C++11 появилось `...`:

``` cpp
template<typename ...Ts> // Группа из типов от 0 до inf
void print_all(const Ts &...value) { // Каждый имеет независимый тип const T&
    (std::cout << ... << value); // C++17: fold exppession
    // Экивалентно
    // std::cout << v1 << v2 << v3;
}
```

Мнемоническое правило- если вводится `parameter pack`, то многоточия ставятся слева от того, что объявили.

Важно, что `parameter pack` объявляется в конце списка шаблонных параметров, чтобы было понятно, где заканчивается список параметров.

Чтобы узнать размер полученного набора переменных, можно использовать
оператор `sizeof...`:

```cpp
template<typename T, typename ...Ts>
struct Foo {
    std::size_t size = sizeof...(Ts);
}
```

Отметим, что создать тип `Ts...`  нельзя, но этот список параметров можно передавать, например, в `std::tuple`:

```cpp
template<typename ...Ts>
struct Foo {
    std::tuple<Ts...> container;
}
```

Также, можно объявлять не только `typename`, но и сам тип. Например, можно сделать список целых чисел:

```cpp
template<int ...Ts>
struct Foo {
}
```

Можно это использовать, чтобы сохранить список аргументов,
если значения известны на этапе компиляции:

```cpp
template<typename ...Ts>
struct Foo{
    template<Ts ...values> struct Bar{};
};

Foo<int, int(*)()>::Bar<10, func> x;
```

Есть применение и в наследовании. Например:

```cpp
struct Bar{
    Bar(int value) {}
    int func(int);
};
struct Baz{
    Bar(int value) {}
    int func(char);
};

template<typename ...Ts>
struct Foo: Ts... {
    Foo(int value) : Ts(value)... {}
    
    // С C++17, чтобы все функции были видны как перегрузки друг друга,
    //                                          можно использовать:
    using Ts::func...; 
};

Foo<Bar, Baz> x(10);
```

Начиная с C++17 можно применять и `auto` похожим образом:

```cpp
template<auto ...Ts> struct Foo{};
Foo<10, func, func> x;
```

Рассмотрим еще один пример:

```cpp
template<typename ...Ts>
int sum_all(const Ts &...value) {
    return (10 + ... + value);
    // return (value + ... + 10);
    // Скобки важны, так как fold expression должен быть 
    //                                  отдельный выражением
}
```

Такая конструкция хороша тем, что можно вызвать `sum_all()` от нуля аргументов.
Однако, следующую функцию вызвать так же от нуля аргументов не получится:

```cpp
template<typename ...Ts>
void sum_all(const Ts &...value) { 
    return (... + value);
}
```

### Fold expressions ###

C++17.
Поитерируемся по данным аргументам.  Для этого применяется оператор `','`.
Напомним, что оператор `','` в выражении `(a, b)` выполняет сначала `a`, потом `b`
и возвращает результат `b`.

```cpp
template<typename ...Ts>
void print_all(const Ts &...args) {
    ((std::cout << args << "\n"), ...); // fold expression
    /* Раскрывается в
        * std::cout << arg1 << "\n"
        * std::cout << arg2 << "\n"
    */
}
```

Важен синтаксис `((std::cout << args << "\n"), ...)`:

* `(std::cout << args << "\n"),  ...` - fold expression не может быть на самом верхнем уровне (сразу под ;)
* `(std::cout << args << "\n", ...)` - смешаны операторы и не получается распарсить fold expression

Альтернативный способ написать то, что выше:

```cpp
template<typename ...Ts>
void print_all(const Ts &...args) {
    auto f = [](auto arg) {
        std::cout << arg << "\n"  
    }
    (f(args), ...);
}
```

Обратите внимание, что auto в объявлении лямбды принципиально, так как могут быть аргументы разных типов.

Научимся еще аргументы передавать:

```cpp
template<typename ...Ts>
void print_all(const Ts &...args) {
    auto f = []() {
        std::cout << args << "\n"  
    }
    (f(args), ...);
}

template<typename ...Ts>
void print_all_increased(const Ts &...args) {
    print_all(">", args..., "<", args..., "!");
    print_all(">", args..., "<", (args + 10)... , "!"); 
    // Запись (args + 10)... добавляем к каждому элементу из args 10
}
```

Способ проверить типы: `static_assert((... && std::is_same_v<Ts, int>));`,
начиная с C++20- requires после объявления шаблонных параметров в сигнатуре.

Можно менять ассоциативность при развертывании аргументов:

* `(args + ...) = arg0 + (arg1 + arg2)`
* `(... + args) = (arg0 + arg1) + arg2`
* Для унарного `fold expression`  требуется непустой args, кроме операторов `&& || ,`
* Бинарный `(0 + ... + args)` может работать и с пустым
* Аналогично с ассоциативностью: `(args + ... + 0) = arg0 + (arg1 + 0)`
* `(0 + ... + args) = (0 + arg0) + arg1`

## Tuple ##

Чтобы написать `std::tuple` воспользуемся частичной специализацией шаблонов.

```cpp
template<typename ...Ts> struct tuple{};

template<typename T, typename ...Args> 
struct tuple<T, Args...> {
    T head;
    tuple<Args...> tail;
} inst;
```

Чтобы узнать размер `tuple` создадим вспомогательную метафункцию:

```cpp
template<typename> struct tuple_size {};
template<typename ...Args>
struct tuple_size<tuple<Args...>>
    : std::integral_constant<std::size_t, sizeof...(Args)> {};
```

`std::integral_constant` хранит поле `value` типа `std::size_t`, в которое
записывается значение размера структуры.

Также можно написать функции `tuple_element`, которая по номеру элемента
выдает его тип, и `get`- выдает значение элемента:

```cpp
template<std::size_t, typename> struct tuple_element;

template<typename Head, typename ...Tail>
struct tuple_element<0, tuple<Head, Tail...>> {
    using type = Head;
};

template<std::size_t I, typename Head, typename ...Tail>
struct tuple_element<I, tuple<Head, Tail...>> :  
    tuple_element<I - 1, tuple<Tail...>> {
};

tuple_element<0, tuple<int, string>>::type; // = int
tuple_element<1, tuple<int, string>>::type; // = string
```

```cpp
template<std::size_t I, typename ...Ts>
auto get(const tuple<Ts...> &tuple) {
    if constexpr (I == 0) return tuple.head;
    else                  return get<I - 1>(tuple.tail); 
}
```

Здесь используется `if constexpr` (с C++17). Этот `if` позволяет не
только проверять константые условия, но и компилировать только одну из веток.

## Perfect forwarding ##

Положим, есть функция, выполняющая какой-то алгоритм, и функция,
которая считает время его выполнения:

```cpp
void calc(int n) {
    // Алгоритм
}

template<typename Fn, typename Arg0>
void timed(Fn fn, Arg0 arg0) {
    auto start = std::chrono::steady_clock::now();
    // Часы, которые идут только вперед
    fn(std::move(arg0));
    auto duration = std::chrono::steady_clock::now() - start;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
                    (duration).count() << "ms\n";
}
```

Хотим научиться передавать в алгорит дополнительный флаг `completed`.
Модифицируем имеющийся код:

```cpp
void calc(int n, bool &completed) {
    // Алгоритм
}

template<typename Fn, typename Arg0, typename Arg1>
void timed(Fn fn, Arg0 arg0, Arg1 &arg1) {
    auto start = std::chrono::steady_clock::now();
    fn(std::move(arg0), arg1);
    auto duration = std::chrono::steady_clock::now() - start;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
                    (duration).count() << "ms\n";
}
```

Такая реализация не очень удобна, потому что в `timed()` приходится передавать тот же аргумент по ссылке. Хотим, чтобы функция сама догадалась, что нужно передать по ссылке.
С другой стороны, проблема, что пока это тяжело обощается на variadic templates,
потому что все будет передаваться либо по значению, либо по ссылке.
Более того, функция `timed()` теперь своя на каждый вид функций. Хочется, чтобы
она не зависела от того, как `fn` принимает аргументы.

Далее описана некоторая магия, которая называется [perfect forwarding](https://habr.com/ru/post/242639/):

```cpp
void calc(int n, bool &completed) {
}

template<typename Fn, typename Arg0, typename Arg1>
void timed(Fn fn, Arg0 &&arg0, Arg1 &&arg1) {
    auto start = std::chrono::steady_clock::now(); 
    fn(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1));
    auto duration = std::chrono::steady_clock::now() - start;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
                    (duration).count() << "ms\n";
}
```

Заметим, что в сигнатуре функции `void timed(Fn fn, Arg0 &&arg0, Arg1 &&arg1)`
`arg1` и `arg2` принимаются не по rvalue-ссылке (костыли специальны для этого случая,
&& именно в шаблонах).
И как результат, `fn(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1));`
начинает передавать аргументы, которые нужно передавать по lvalue ссылке- по lvalue ссылке,
а те, которые нужно передать rvalue - по rvalue. Работает
даже const-qualifiers.

Чтобы разобраться с тем, как это работает, надо посмотреть в правило вывода типов.

```cpp
template<typename> struct TD; // Type Display

template<typename T>
void foo(T &x) {
    TD<T>(); 
    /* 
     * При попытке инстанцировать объект этого типа 
     * происходит ошибка компиляции (incomplete type). 
     * Это ровно то, что нужно, так как тогда можно узнать,
     * чему равен тип T.
    */
    TD<decltype(x)>();
}

int main() {
    int v = 20;
    const int cv = 30;

    foo(10);
    foo(v);
    foo(cv);
}
```

* `foo(10)` - не скоплируется, так как передаем временный объект
* `foo(v)` - вывелось в `int`
* `foo(cv)` - вывелось в `const int` - интуция как `auto`, чтобы можно было привязать константую ссылку к cv

Однако, в C++11 появился отдельный случай `forwarding reference`:

```cpp
template<typename> struct TD;

template<typename T>
void foo(T &&x) { // forwarding reference
    TD<T>(); 
    TD<decltype(x)>();
}

int main() {
    int v = 20;
    const int cv = 30;

    foo(10);
    foo(v);
    foo(cv);
    foo(std::move(v));
    foo(std::move(cv));
}
```

Появляются специальные правила:

* `foo(10)`;- вывелось в `int`, тип `x` получился `int&&`
* `foo(v)`; - вывелось в `int&`, тип `x` получился `int&`
* `foo(cv)`; - вывелось в `const int&`, тип `x` получился так же `const int&`
* `foo(std::move(v));` - вывелось в `int`, тип `x` получился `int&&`, что логично (как в первом случае)
* `foo(std::move(cv));` - вывелось в `const int`, тип `x` получился `const int&&`

Итого, получили, что forwarding reference превращается либо в rvalue-ссылку, либо в lvalue-ссылку. Это называеся "правилом сворачивания ссылок":

* `(U&) &` -> `U&`
* `(U&) &&` -> `U&`
* `(U&&) &` -> `U&`
* `(U&&) &&` -> `U&&`

То есть как только где-то возникла невременность объекта, то она там и осталась.
Таким образом, внутри `foo` есть механизм, позволяющий принять аргумент по ссылке
и внутри помнить, как именно мы его приняли. Теперь везде, где мы хотим передать
аргумент "идельно" дальше, то мы можем сделать это, посмотрев на тип T.

Теперь разберемся, как же работает "некоторая магия" из примера выше.
Заменим `std::forward` на `static_cast`:

```cpp
void calc(int n, bool &completed) {
}

template<typename Fn, typename Arg0, typename Arg1>
void timed(Fn fn, Arg0 &&arg0, Arg1 &&arg1) {
    auto start = std::chrono::steady_clock::now();
    fn(static_cast<Arg0&&>(arg0), static_cast<Arg1&&>(arg1));
    auto duration = std::chrono::steady_clock::now() - start;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
                    (duration).count() << "ms\n";
}
```

Уже знаем, что при вызове `timed` значение `500` типа `Arg0` приведется к
типу `int`, а `completed` - не временное значение, поэтому `Arg1` приведется к `bool&`. Далее мы хотим сделать `move` переменной типа `Arg0` (`static_cast`),
что и получается, так как `Arg0&& = int&&`. Для `Arg1&& = (bool&)&& = bool&`, то есть
передали как обычную lvalue-ссылку, что и требовалось.

`std::forward<Arg0>(arg0)` - синтаксический сахар над `static_cast<Arg0&&>(arg0)`.
Отличие от `std::move` - у `std::move` не принято указывать шаблонный параметр, у
`std::forward()` принято, причем:

* `std::forward<Arg1>(arg1) = static_cast<Arg1&>(arg1);`
* `std::forward(arg1) = static_cast<Arg1&&>(arg1);`

Главная идея в том, чтобы принять все аргументы по ссылкам lvalue или rvalue, а потом
передать их дальше, с сохранением категории lvalue-rvalue.

## ref/cref ##

### Некоторая мотивация ###

Пусть есть какой поток, который исполняет функцию worker,
в которую переданы параметры:

```cpp
void worker(int x, int y) {
    cout << x << " " << y;
}

int main() {
    int y = 20;
    std::thread t(worker, 10, y);
    t.join();
    t += 5;
}
```

После исполнения кода выше будет выведено `10 20`, так как
все значения переменных копируются внутрь потока. Однако, что
если есть необходимость передать ссылку на переменную внутрь потока?
Сделать при помощи `void worker(int x, int& y)` нельзя, так как тогда появляются какие-то разделяемые данные с потоком, а это вероятнее всего не то, что хотелось.

### Решение ###

Для решения этой проблемы есть конструкция `std::ref`. `std::ref` возвращает конструкцию `std::reference_wrapper`, которая хранит
в себе указатель на переденное значение и неявно конвертируется в
ссылку на переменную.

```cpp
void worker(int x, int &y) {
    y += 5;
    cout << x << " " << y;
}

std::thread t(worker, 10, std::ref(y));
```

Для константых ссылок есть `std::cref`:

```cpp
void worker(int x, int &y, const int &z) {
    y += 5;
    cout << x << " " << y << " " << z;
}


std::thread t(worker, 10, std::ref(y), std::cref(z));
    
/* Так тоже сработает, потому что автовывод поймет,
 *                              что это ссылка на константу
 * std::thread t(worker, 10, std::ref(y), std::ref(z)); 
*/

/* И так тоже- значение просто скопируется
 * std::thread t(worker, 10, std::ref(y), z);
*/

```

В случае `perfect forwarding` все аргументы передаются по ссылкам
и это хорошо, когда функция вызывается немедленно.

Нужность копирования переменных внутрь потока проявляется в случаях, когда функцию нужно вызвать потом. Тогда нет
никаких гарантий, что временные объекты, ссылки на которые были переданы,
будут жить, когда потребуется вызвать функцию. Тогда конвенция, что
все переменные должны копироватьсся, а если нет, то это необходимо явно указать.

Так же `ref` и `cref` могут использоваться в STL-алгоритмах.
Рассмотрим следующий код:

```cpp
int main {
    struct () {
        int state = 100;
        operator()(int value) {
            std::cout << "state=" << state << ", value=" << value << " ";
            state++;
        }   
    } functor;

    std::vector<int> vec{10, 20, 30};
    std::for_each(vec.begin(), vec.end(), functor);
    std::cout << functor.state << "\n";
}
```

Как результат, будет выведено `100, 101, 102, 100`.
То есть, функтор скопировался внутрь функции. Если
есть цель сделать так, чтобы функтор менялся, то
можно сделать вызов `std::for_each(vec.begin(), vec.end(), std::ref(functor));`

Из рассуждений выше можно было бы сделать вывод, что
всегда можно было бы использовать `ref`, однако если есть цель навесить
[константность](https://stackoverflow.com/questions/26625533/stl-ref-and-cref-functions) при передаче аргумента в функцию, то
можно использовать `cref` вместо более громоздких конструкций.

## `decltype` и `decltype(auto)` ##

Помимо того, что иногда нам надо уметь делать "идеальную передачу" аргументов в некоторую функцию, порой нам хочется
делать "идеальное возвращение" значения, которое возвращает функция (например, если она возвращает значение, то вернём
значение, а если возвращает по ссылке, то и вернём мы из нашей обёртки тоже ссылку)

```c++
template <typename F, typename ...Ts>
   /*непонятно, что тут написать*/ logged(F f, Ts &&...args) {
   std::cout << "Before function invocation" << std::endl;
   return f(std::forward<Ts>(args)...);
}
```

Можно вспомнить про `auto`, но, к сожалению, он тут не поможет:

> Type deduction drops references

> Type deduction drops const qualifiers

Чтобы реализовать идеальное возвращение значения из функции, нужно понять, как работает `decltype`, который мы уже в
какой-то момент встречали (например, когда разговаривали про функторы и лямды)

```c++
int x = 10;
int &y = x;

decltype(10); // int
decltype(10 + 20); // int
decltype(x); // int
decltype(y); // int &
decltype(y + 20); // int
decltype((x)); // int &
```

Особое удивление наверняка вызывает `decltype((x))`, однако это вполне объяснимо, учитывая как работает `decltype`

### На самом деле, у него есть 2 режима работы:

1. `decltype(expression)` (вызываем `decltype` от выражения)
    1. `decltype(10)`
    2. `decltype((x))`
    3. `decltype(x + 20)`
2. `decltype(variable)` (вызываем `decltype` от какой-то переменной)
    1. `decltype(x)`

**В первом случае** он работает следующим образом:

1. Если то, что возвращает выражение -- это `prvalue`, то `decltype` нам вернёт сам тип `T`

2. Если то, что возвращает выражение -- это `lvalue`, то `decltype` нам вернёт `T&`

3. Если то, что возвращает выражение -- это `xvalue`, то `decltype` нам вернёт `T&&`

По сути, отдельно считаем тип и категорию выражения и из этого собираем тип, который возвращает

**Во втором случае** он работает очень просто: смотрит, с каким типом переменная была объявлена

Вот небольшие примеры того, как `auto` и `decltype(auto)` работают в выводе возвращаемого значения функции

```c++
int x;

auto foo() {
   // --> int foo()
   return x;
}

decltype(auto) bar() {
   // --> int bar (выводится int, так как 'x' был объявлен c типом int)
   return x;
}

decltype(auto) baz() {
   // --> int & baz() (выводится int&, так как тип выводится по правилам decltype)
   // это выражение
   return (x);
}

```

Напишем, как теперь выглядит идеальное возвращение значения со знанием правил работы `decltype`

```c++
template <typename F, typename ...Ts>
decltype(auto) logged(F f, Ts &&...args) {
   std::cout << "Before function invocation" << std::endl;
   return f(std::forward<Ts>(args)...);
}
```

А ещё всё это можно написать в виде лямбды

```c++
auto logged =[&](auto f, auto &&...args) -> decltype(auto) {
   std::cout << "Before function invocation" << std::endl;
   return f(std::forward<decltype(args)>(args)...);
};
```

1. [constexpr-функции-на-этапе-компиляции](#constexpr-функции-на-этапе-компиляции)
1. [Функции-из-типов-в-типы](#функции-из-типов-в-типы)
1. [type-traits](#type-traits)
1. [оператор-noexcept](#оператор-noexcept)
1. [type-display-trick](#type-display-trick)
1. [addressof](#addressof)

## constexpr-функции-на-этапе-компиляции
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
    * Можно вызывать другие `constexpr` функции.
    * Можно использовать только `return`, все пишется рекурсивно.
* C++14
    * Можно писать циклы и заводить переменные. Также можно изменять переменные.
    * Можно использовать примитивные типы и типы с `constexpr`-конструктором и примитивным деструктором.
* До C++20
    * Нельзя использовать динамическое выделение памяти (как следствие нельзя использовать `std::vector`)
      Если из `constexpr`-функции выкинуть исключение, то здесь возможно 2 варианта:
    * Исключение было выкинуто на стадии компиляции (функцию вызвали на стадии компиляции), тогда просто произойдет ошибка компиляции.
    * Функцию вызвали в runtime -- исключение ведет себя обычным образом.


## Функции-из-типов-в-типы
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
Как-то очень громоздко, чтобы это сокращать (а именно не писать `::value_type`) можно писать шаблонные псевдонимы. Их принято писать с суффиксом `_t` (если мы по типу получаем другой тип) или с суффиксом `_v` (если мы по типу получаем значение).
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
namespace std {
template <typename T>
struct iterator_traits <MyMagicIterator<T>> {
	using value_type = typename MyMagicIterator<T>::magic_value_type;
	//....
};
}
```
Еще есть `std::char_traits<T>`, которые позволяют работать со всякими символьными типами.
Например если мы хотим сделать свой класс, который сравнивает 2 символа вне зависимости от регистра, то мы получим что-то такое:

```C++

struct case_insensitive_traits : char_traits<char> {
	static bool lt(char a, char b) {return tolower(a) < tolower(b)};
	static bool eq(char a, char b) {return tolower(a) == tolower(b)};
	//...
}; 

using case_insensitive_string = std::basic_string<char, case_insensitive_traits>;
```

Обычно свои `char_traits` это ужас, и `tolower` -- древняя штука из C, ее тоже не используйте.

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

## оператор-noexcept

Этот оператор позволяет по выражению (не вычисляя его) проверить, что оно не кидает исключений (а это зависит от спецификаторов `noexcept`).
Пример:

```C++
constexpr int a = 11;
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
Только вот T необязательно констурируется по умолчанию, и мы не хотим проверять, что конструктор по умолчанию noexcept. Поэтому есть `std::declval<T>()`, который можно использовать только внутри _невычислимых контекстов_:

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

## type-display-trick
Когда метапрограммируем, обычно хочется узнавать типы переменных (например для отладки) со всеми ссылками и константностью. Тогда можно сделать вот такой трюк и узнать тип переменной по CE:

```C++
template <typename T> struct TD; // только forward-declaration

TD<decltype(foo())>{}; //будет подробная ошибка
```

## addressof
Для справки: оператор `&a` (взятия адреса) именно оператор, то есть его можно перегрузить. А когда вы создаете библиотеку, то должны создавать код, который работает во всех случаях (даже когда пользователь не очень умный и зачем-то перегрузил этот оператор).  Для этого есть стандартный `std::addressof`. Предпологаемая упрощенная реализация такая:

```C++
template <typename T>
T *addressof(T &value) {
	return reinterpret_cast<T *>(&reinterpret_cast<char&>(value)); //Все таки у char нельзя перегрузить этот оператор.
}
```
Еще нужно сделать несколько перегрузок для константых случаев, но тут этого нет.


## Указатель на члены класса

Иногда может хотеться сохранить указатель на какой-то член класса в переменную.

Рассмотрим пример, как такое можно сделать

```c++
struct S {
  int x = 1;
  int y = 2;
  int foo(int t) { return x + y + t; }
  int bar(int t) const { return x + y + t + 10; }
};

S obj;
// создаём указатель a на поле 'x' класса S
int (S::*a) = &S::x;
// создаём указатель b на поле 'y' класса S
int (S::*b) = &S::y;
```

Синтаксис, безусловно, крайне упоротый. Но это ещё не всё! Вот как можно обращаться к полю, которое лежит по этому
указателю

```c++
int x1 = obj.*a;
S *objptr = &obj;
int y1 = objptr->*b;
```

Кстати, их тоже можно переприсваивать

```c++
a = b;
// теперь мы так обращаемся к полю 'y', а не 'x'
obj.*a;
```

В целом, аналогично полям можно работать с указателями на методы класса

```c++
int (S::*fooptr)(int) = &S::foo;
std::cout << (obj.*fooptr)(10) << std::endl;
```

**С указателями на методы важно не забывать сохранять const-qualifier**

```c++
int (S::*barptr)(int) const = &S::bar;
```

Кстати, возникают все стандартные проблемы с перегрузками.

Это всё, конечно, очень задорно и весело, можно пугать детей и всё такое, но **зачем???**

Ну оказывается, что с помощью этого механизма удобно писать какие-нибудь сериализаторы.

Кстати говоря, указатели на члены несовместимы с обычными указателями

1. Указатель на поле -- это сдвиг внутри объекта. Это не `void*` и не указатель на объект
2. Указатель на метод несовместим даже с обычными указателями на функцию
    1. Бывают виртуальные функции и виртуальное наследование
    2. Надо поддерживать преобразования по иерархии


## SFINAE

Для начала, **SFINAE** расшифровывается как **S**ubstitution **F**ailure **I**s **N**ot **A**n **E**rror

Общий смысл таков: если при подстановке шаблонных параметров в сигнатуру функции произошла ошибка, то компилятор просто
считает, что этой перегрузки как бы нет. Моментальной ошибки компиляции не происходит,
она может возникнуть потом, если ни одна перегрузка не подойдёт.

Например, это вполне валидный пример того, что SFINAE действительно работает

```c++
template<typename T>
void duplicate_element(T &container, typename T::iterator iter) {
    container.insert(iter, *iter);
}

template<typename T>
void duplicate_element(T *array, T *element) {
   assert(array != element);
   *(element - 1) = *element;
}

std::vector a{1, 2, 3};
duplicate_element(a, a.begin() + 1);
int b[] = {1, 2, 3};
duplicate_element(b, b + 1); // Нет ошибки, когда пробуем первую перегрузку: не бывает int[]::iterator, но это не ошибка компиляции. SFINAE.
```

Вот ещё пример корректного использования SFINAE

```c++
struct BotvaHolder {
    using botva = int;
};

template<typename T> using GetBotva = typename T::botva;

template<typename T>
void foo(T, GetBotva<T>) {  // Просто псевдоним, так можно, проблемы возникают как бы "тут".
    std::cout << "1\n";
}

template<typename T>
void foo(T, std::nullptr_t) {
    std::cout << "2\n";
}

 foo(BotvaHolder(), 10);  // 1
 foo(BotvaHolder(), nullptr);  // 2
 // foo(10, 10);  // CE
 foo(10, nullptr); // 2
```

Тут в случае, когда мы подставляем std::nullptr_t в качестве типа `T`, происходит ошибка при подстановке в сигнатуру
функции, ведь у std::nullptr_t нет внутреннего типа `botva`.

А в случае с `foo(10, 10)` не получится найти ни одной корректной перегрузки и поэтому произойдёт CE.

Теперь рассмотрим случай, когда задуманное не работает, то есть, когда мы думаем, что используем SFINAE, но, оказывается, что не совсем.

```c++
struct BotvaHolder {
    using botva = int;
};

template<typename T>
struct GetBotva {
    using type = typename T::botva;  // hard compilation error.
};

template<typename T>
void foo(T, typename GetBotva<T>::type) {  // Ошибка компиляции, потому что проблема возникла уже внутри GetBotva.
    std::cout << "1\n";
}

template<typename T>
void foo(T, std::nullptr_t) {
    std::cout << "2\n";
}

 foo(BotvaHolder(), 10);  // 1
 foo(BotvaHolder(), nullptr);  // 2
 // foo(10, 10);  // CE
 foo(10, nullptr); // 2
```

В данном случае у нас не работает SFINAE, так как в сравнении с предыдущим примером
у нас ошибка произойдёт при попытке инстанцировать шаблонную структуру `GetBotva` on `int`.
То есть, идея в том, что мы уже попытаемся внутри этой структуры залезть в `int::botva`,
что вызовет ошибку компиляции, но это не приведёт к тому, что эта перегрузка просто не будет рассмотрена.
Это приведёт к моментальной ошибке компиляции, чего мы вряд ли хотели

## `enable_if`

Это одна из штук, которые очень активно используются в метапрограммировании.

Пусть у нас есть какая-то возможность посчитать булево значение во время компиляции и в зависимости от вычисленного 
значения разрешать или запрещать какие-то перегрузки шаблонных функций

Для такого есть очень удобная вещь из стандартной библиотеки: `std::enable_if_t<bool, type>`, которая равна `type`, 
если условие, которое передаётся первым параметром шаблона, верно. 

Посмотрим на реализацию, она очень простая

```c++
template<bool Value, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> {
    using type = T;
};

template<bool Value, typename T = void>
using enable_if_t = typename enable_if<Value, T>::type;
```

Чтобы лучше понять, как оно работает и где нужно, можно посмотреть примеры кода ниже

```c++
template<typename T>
auto print_integral(std::ostream &os, const T &value) -> std::enable_if_t<std::is_integral_v<T>> {
    os << value;
}
```

Вот, например, функция, в которую мы ожидаем на вход целочисленный тип. Тогда можно не передавать тип вторым параметром шаблона (там по умолчанию как раз `void`, что логично в нашем случае). Если в функцию передали
действительно целое число, то всё будет хорошо, будет успешная подстановка типов, выведется тип возвращаемого значения 
как `void`. А вот если вдруг мы попытаемся передать туда что-то другое, то сработает механизм SFINAE, поскольку не
получится получить доступ к внутреннему типу `type` из-за того, что условие, переданное первым шаблонным параметром,
`false`. А для такого случая у структуры `enable_if` этого внутреннего типа нет.

А вот ещё пример: пусть есть функция, которой нужно, чтобы переданные аргументы имели перегруженный оператор `+`

```c++
template<typename T>
auto sum(const T &a, const T &b) -> decltype(a + b) {
    return a + b;
}
```

Если у типа `T` он не будет перегружен, то при подстановке `T` в сигнатуру произойдёт ошибка. SFINAE сработает.

А вот ещё классный пример, тут мы делаем так, что при некоторых значениях типа `T` структура будет содержать метод 
`get_integral`, а при некоторых -- нет


```c++
template<typename T>
struct wrapper {
    T value;

    wrapper(T value_) : value(std::move(value_)) {}

    T &get() { return value; }
    const T &get() const { return value; }

    template<typename U = T>
    std::enable_if_t<std::is_integral_v<U>, U> get_integral() {
        return value;
    }
};
```

То есть, если мы объявим переменную типа `wrapper<int>`, то нам будет доступен `get_integral`, а если объявим, скажем, 
`wrapper<double>`, то он доступен уже не будет. На самом деле, можно добиться аналогичного поведения, используя 
специализации, но там бы пришлось копипастить. Тут не приходится. Кстати, тут очень важно, что `is_integral` шаблонная 
функция, если бы вместо этого просто написали `std::enable_if_t<std::is_integral_v<T>, T> get_integral`, то ошибка 
произошла бы при инстанцировании этой шаблонной структуры с любым нецелочисленным типом `T`. Что вновь не совсем то 
поведение, которого мы хотели добиться. 

