# Метапрограммирование #

## Небольшое введение ##

Что такое метапрограммирование?
В общих словах, это когда вы пытаетесь заставить комиплятор написать программу за вас. Например, шаблоны- вполне себе метапрограммирование.

## Parameter pack и variadic template ##

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

Можно это использовать, чтобы сохранить список аргументов (альтернатива `std::tuple`):

```cpp
template<typename ...Ts>
struct Foo{
    template<Ts ...values> Bar{};
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
void sum_all(const Ts &...value) {
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

* `(std::cout << args << "\n"),  ...` - компилятор не поймет, что это выражение.
* `(std::cout << args << "\n", ...)` - компилятор не сможет
обработать выражение.

Альтернативный способ написать то, что выше:

```cpp
template<typename ...Ts>
void print_all(const Ts &...args) {
    auto f = []() {
        std::cout << args << "\n"  
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
* `(... + args) = (arg2 + arg1) + arg0`
* Для унарного `fold expression`  требуется непустой args, кроме операторов `&& || ,`
* Бинарный `(0 + ... + args)` может работать и с пустым
* Аналогично с ассоциативностью: `(args + ... + 0) = arg0 + (arg1 + 0)`
* `(0 + ... + args) = (0 + args) + arg1`

## Tuple ##

Чтобы написать `std::tuple` воспользуемся частичной реализацией шаблонов.

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
начинает передавать аргументы, которые нужно передавать по ссылке- по ссылке,
а те, которые нужно передать по значению - по значению. Работает
даже с rvalue ссылками и const-qualifiers.

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

Итого, получили, что forwarding reference превращается либо в rvalue-ссылку, либо в lvalue-ссылку. Это называеся "правилом сворачивания ссылок" ("reference collapsing"):

* `(U&) &` -> `U&`
* `(U&) &&` -> `U&`
* `(U&&) &` -> `U&`
* `(U&&) &&` -> `U&&`

То есть как только где-то возникла временность объекта, то она там и осталась.
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
