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

## Proxy && vector bool

* Если написать что-то вроде:
```c++
int main() {
    std::vector<int> vi(100);
    std::vector<bool> vb(100);
    vb[2] = false;
    if (vb[3]) {
    	...
    }
}
```
то никаких проблем не будет.
* Есть прикольный факт, что `vector<bool>` выделяет `n/8` байт памяти. Из-за этого возникает много забавных проблем.
* Рассмотрим пример с прокси-объектом:
```c++
struct byte {
private:
    char data = 0;

    struct Proxy {
        char &data;
        int bit;

        /*explicit*/ operator bool() {  // Try uncommenting and see what breaks.
            return data & (1 << bit);
        }

        Proxy& operator=(bool new_value) {
            if (new_value) {
                data |= 1 << bit;
            } else {
                data &= ~(1 << bit);
            }
            return *this;
        }
    };

public:
    bool operator[](int bit) const {
        return data & (1 << bit);
    }

    Proxy operator[](int bit) {
        return Proxy{data, bit};
    }

    explicit operator char() const {
        return data;
    }
};
```
тогда если мы напишем следующее:
```c++
byte x;
x[2] = true;
std::cout << static_cast<int>(static_cast<char>(x)) << "\n";
```
то вывод: `4`.
* Наш `Proxy` класс не виден из вне, а возникает только тогда, когда мы используем `[]`. 
* Он хранит ссылку на `char`, который нужно поменять и номер бита, который нужно поменять. И менят нужный бит в нужном `char`.
* В `C++11` возникает много различных проблем.
* `Proxy` не является нормальным `bool`, то есть по умолчанию нельзя написать `bool b = x[2]`, использовать в ифах или выводить; но эта проблема обходится в помощью`operator bool()`.
* `explicit` испортит `cout`.
* Рассмотрим код:
```c++
#include <vector>
#include <iostream>

template<typename T>
void test() {
    std::vector<T> vec(100);

    std::cout << vec[10] << "\n";
    {
        auto x = vec[10];
        x = 1;
        std::cout << x << "\n";
    }
    std::cout << vec[10] << "\n";
}

int main() {
    test<int>();
    test<bool>();
}
```
для `int` и `bool` работает по разному. (Для `bool` - значение поменяется).
* Код выше для всех, кроме `bool` аботает правильно.
* А также, нельзя писать `auto &x = vec[10];` 
* Итого, из-за решения экономить память, возникает много проблем.

## Тип "функция"
* Такой тип имеют функции:
  ```c++
  void func1();
  void func2(int x);
  void func3(int y);
  int func4(int x, char y);
  // ....
  TD<decltype(func1)>{}; // Ошибка компиляции: TD<void()> is incomplete
  TD<decltype(func2)>{}; // Ошибка компиляции: TD<void(int)> is incomplete
  TD<decltype(func3)>{}; // Ошибка компиляции: TD<void(int)> is incomplete
  TD<decltype(func4)>{}; // Ошибка компиляции: TD<int(int, char)> is incomplete
  ```
* Объявить переменную такого типа нельзя.
* В метапрограммировании обычно применяется для упрощения синтаксиса: `function<void(int)>`.
* Неявно приводится к _указателю_ на функцию.

---
## Тип "указатель на функцию"
Добавили между типом и аргументами `(*)`, получили `void(*)(int)`.

```c++
char foo(int);
// ....
auto x = [](int val) -> char { return val; };
auto y = [&](int val) -> char { return val; };
char (*func1)(int) = &foo;  // Ок.
char (*func2)(int) = foo;   // Ок, функция неявно преобразуется в указатель на себя.
char (*func3)(int) = x;     // Ок: лямбда без захвата — почти свободная функция.
char (*func4)(int) = y;     // Не ок: лямбда с захватом должна знать своё состояние.
```

А ещё функции и данные могут вообще в совсем разной памяти лежать:
```c++
void *x = static_cast<void*>(&foo);  // Ошибка компиляции.
```
* Но Linux пофиг: там какие-то функции возвращают `void*` вместо указателей на функции.

Между собой указатели на функции несовместимы:
```c++
void (*func5)() = func4;
void (*func6)(int) = func4;
```

---
## Указатели на шаблонные функции
В прошлом семестре мы уже передавали указатели на функции в нешаблонную функцию. Давайте сделаем её шаблонной:
```c++
template<typename T>
void apply(void (*operation)(T), T data) { // cdecl.org
operation(data);
}

template<typename T>
void apply10(void (*operation)(T)) { // cdecl.org
operation(10);
}

template<typename T>
void print(T x) {
std::cout << x << "\n";
}

void print_twice(int x) {
std::cout << x << ", " << x << "\n";
}

template<typename T>
void (*print_ptr)(T) = print;

int main() {
apply<int>(print, 10); //ок, T = int
apply<int>(print_twice, 20); // ок, T = int

apply(print, 10); // ок, автовывод типа по второму аргументу
apply(print_twice, 20); // ок по тем же причинам

apply10(print); // не смогли вывести тип T, ошибка компиляции
apply10(print_twice); // ок

void (*ptr1)(int) = print; // ок, завели переменную указатель на функцию void с одним параметром int, инстанцируем шаблон
void (*ptr2)(double) = print; // ок 
ptr1 = print_twice; // ок

auto ptr3 = print_twice; // ок, print_twice - не шаблонная
auto ptr4 = print; // не ок, print - шаблонная, с каким T нужно - непонятно :(

void (*ptr5)(int) = print_ptr<int>; // ок
}
```
## Указатели на перегруженные функции
```c++
void foo(int x);
char foo(char x);
// ....
TD<decltype(&foo)>{};  // Ошибка компиляции: какую перегрузку выбрать decltype?
```

Вывод: у `&foo` нет типа (примерно как у `{1, 2, 3}`).

```c++
void (*func1)(int)  = &foo;  // Если в точности указать тип, то перегрузка разрешится.
char (*func2)(char) = &foo;
```

Вызывает проблемы в шаблонных функциях или с `auto`:
```c++
template<typename Fn> void bar(Fn fn) { fn(10); }
// ....
bar(&foo);  // no matching function for call to 'bar(<unresolved overloaded function>)'
bar(static_cast<void(*)(int)>(&foo));  // Ок.
bar([](int x) { return foo(x); });     // Ок, у лямбды фиксированный параметр.
```
Ещё пример:
```c++
template<typename T>
void apply(void (*operation)(T), T data) { // cdecl.org
    operation(data);
}

template<typename T>
void apply10(void (*operation)(T)) { // cdecl.org
    operation(10);
}

void print(int x) {
    std::cout << x << "\n";
}

void print(double x) {
    std::cout << x << "\n";
}

int main() {
   apply<int>(print, 10);
   apply<int>(print, 20);

   apply(print, 10); // ок, выводится по второму аргументу

   apply10(print); //не ок, автовывод не работает

   void (*ptr1)(int) = print; // print(int)
   void (*ptr2)(double) = print; // print(double)

   auto ptr4 = print; // не ок, нельзя делать auto из перегуженной функции
   auto ptr5 = static_cast< void(*)(int) >(print); // так можно
   void (*ptr6)(int) = print;
}   
```

## Switch - case

Рассмотрим пример использования:

```c++
switch (x) {
    case 1:
        std::cout << "  1\n";
        break;
    case 4:
        std::cout << "  4\n";
        break;
    case 2:
        std::cout << "  2\n";
        break;
    default:
    	std::cout << " default\n";
    	break;
}
```

* У нас есть несколько кейсов, и в зависимости от значения `x` мы либо перепрыгиваем на соответственный кейс, либо на значение по умолчанию (если есть).
* Объявлять кейсы можно только констаной (хотя `2 + 2` работает)
* Важно не забывать про `break` (будут варнинги), так как иначе он будет прыгать (как `goto`) в нужный `case`, а дальше будет заходить в каждый кейс ниже. То есть `break` - перейди в конец `switch`.
* Для меньшего количества копи-паста можно писать так:
```c++
case 1:
case 10:
```
Работает как `or`
* Команда `[[fallthrough]];` как раз отрубает ворнинги из-за отсутствия `break`:
```c++
case 1:
    std::cout << "  1\n";
    [[fallthrough]];
```
Будет переходить также и в следующий кейс, если `x = 1`.
* Стилистически лучше писать `default` последним, а кейсы - в алфавитном порядке. В последнем кейсе лучше всегда ставить `break;`
* Нужно быть аккуратным с объявлением переменных!
* Рассмотрим пример:
```c++
switch (x) {
    case 1:
        std::cout << "  1\n";
        break;
    case 4:
        std::cout << "  4\n";
        break;
    case 2:
        std::cout << "  2\n";
        break;
    default:
        int wtf1 = 0;
    	std::cout << " default\n";
    	break;
}
```
Тут компилятор не будет ругаться, так как инициализация только в последнем кейсе.
* Если есть инициализация не в последнем кейсе:
```c++
switch (x) {
    case 1:
        std::cout << "  1\n";
        break;
    case 4:
        std::cout << "  4\n";
        break;
    case 2:
        int wtf = 0;
        std::cout << "  2\n";
        break;
    default:
    	std::cout << " default\n";
    	break;
}
```
тогда компилятор будет ругальтся, так как мы будем перепрыгивать через них и на это ругается компилятор.
* Бороться к этим просто - написать фигурные скобки:
```c++
switch (x) {
    case 1:
        std::cout << "  1\n";
        break;
    case 4:
        std::cout << "  4\n";
        break;
    case 2:
    {
        int wtf = 0;
        std::cout << "  2\n";
        break;
    }
    default:
    	std::cout << " default\n";
    	break;
}
```
* что-то похожее написано тут: https://drive.google.com/drive/folders/1WnOi3GJErmxnRlIDLqfOg7wTKHLEFtZG

## Structured binding — базовое
С С++17 можно почти совсем как в Python:

```c++
std::pair<int, string> p(10, "foo");
auto [a, b] = p;  // a == 10, b == "foo"
b += "x";  // b == "foox", p.second == "foo"
```

К `auto` можно добавлять `const`/`&`/`static`:

```c++
auto& [a, b] = p;  // a == 10, b == "foo"
b += "x";  // b == p.second == "foox"
```

* Есть direct initialization: `auto [a, b](p);`.
* Есть list initialization: `auto [a, b]{p};`.
* Указать тип отдельных `a`/`b` нельзя.
* Нельзя вкладывать: `auto [[a, b], c] = ...`.
* Нельзя в полях.
* Происходит на этапе компиляции: можно с массивами, но не с векторами.
* Также работает с очень простыми структурами.

---
## Structured binding — применения
Удобно получать значения `pair` из `.insert`.
```c++
map<int, string> m = ....;
if (auto [it, inserted] = m.emplace(10, "foo"); inserted) {
    cout << "Inserted, value is " << it->second << '\n';
} else {
    cout << "Already exists, value is " << it->second << '\n';
}
```

Удобно итерироваться по `map`.
```c++
for (const auto &[key, value] : m) {
    cout << key << ": " << value << '\n';
}
```

---
## Как работает
```c++
auto [key, value] = *m.begin();
/*                  ^^^EXPR^^^ */
```
превращается в
```c++
// 1. Объявляем невидимую переменную ровно так же.
//    Для примера тут copy initialization.
auto e = *m.begin();  // map<int, string>::value
                      // pair<const int, string>
using E = pair<const int, string>;
// 2. Проверяем количество аргументов.
static_assert(std::tuple_size_v<E> == 2);
// 3. Объявляем элементы.
std::tuple_element_t<0, E> &key   = get<0>(e);  // Или e.get<0>()
std::tuple_element_t<1, E> &value = get<1>(e);  // Или e.get<1>()
```

* На самом деле `key` и `value` — ссылки в невидимый `e`.
* Время жизни такое же, как у `e`.
* Костантность и ссылочность получаем от `tuple_element_t`.
    * В частности, `const auto &[a, b] = foo()` продлит жизнь временному
      объекту.

---
## Подробности structured binding
Поддерживаются три формы привязки:

1. Если массив известного размера:
   ```c++
   Foo arr[3];
   auto [a, b, c] = arr;
   // превращается в
   auto e[3] = { arr[0], arr[1], arr[2] };
   Foo &a = e[0], &b = e[1], &c = e[2];
   ```
* Если не массив, то `tuple_size<>`, `get<>`...
    * Можно предоставить для своего типа, но надо думать про `get`
      от `const` (deep или shallow?) и
      [прочие тонкости](https://stackoverflow.com/questions/61340567).
* Иначе пробуем привязаться ко _всем_ нестатическим полям.
  ```c++
  struct Good { int a, b; }
  struct GoodDerived : Good {};

  struct BadPrivate { int a; private: int b; }  // Приватные запрещены.
  struct BadDerived : Good { int c; }  // Все поля должны быть в одном классе.
  ```

---
## Тонкости structured binding
В зависимости от `auto`/`auto&`/`const auto&` и инициализатора у нас получаются немного разные типы.

* `auto&` попробует привязать ссылку.
* `const auto&` продлит жизнь временному объекту.
* `auto` всегда скопирует объект целиком, а не просто его кусочки.

Если внутри объекта лежали ссылки, то может [сломаться время жизни](https://stackoverflow.com/a/51503253/767632):
```c++
namespace std {
    std::pair<const T&, const T&> minmax(const T&, const T&);
}
auto [min, max] = minmax(10, 20);  // Только копирование значений?
// перешло в
const pair<const int&, const int&> e = {10, 20};
// Сам `e` — не временный, поэтому продления жизни нет.
// e.first и e.second ссылаются на уже умершие 10 и 20.
const int &min = e.first;   // Oops.
const int &max = e.second;  // Oops.
```

Рекомендация: осторожно с функциями, которые возвращают ссылки.
С ними лучше `std::tie`.

---
## Argument-dependent lookup (ADL)
Ссылки: [GotW 30](http://www.gotw.ca/gotw/030.htm), [StackOverflow](https://stackoverflow.com/a/8111750/767632).
Для unqualified lookup: [TotW 49](https://abseil.io/tips/49) и [Cppreference](https://en.cppreference.com/w/cpp/language/adl).

```c++
namespace std {
    ostream& operator<<(ostream &os, string &s);
}
// ....
std::ostream &output = ....;
std::string s;
output << s;  // Почему не std::operator<<(output, s); ?
```
ADL: если видим _неквалифицированный_ вызов функции, то смотрим на типы
аргументов и ищем функции во всех связанных namespace'ах.

Удобно для операторов.

<blockquote style="font-style: italic">
Во-первых, в зависимости от типа аргумента, ADL работает девятью разными способами, убиться веником.
(<a href="https://habr.com/ru/company/jugru/blog/447900/">источник</a>)
</blockquote>

---
## Примеры работающего ADL
[Обобщение](http://www.gotw.ca/publications/mill02.htm): если мы вместе с классом дали пользователю какую-то функцию,
то она должна иметь те же моральные права, что и методы.

```c++
std::filesystem::path from("a.txt");  // C++17
std::filesystem::path to("a-copy.txt");
copy_file(from, to);  // copy_file не метод, ей не нужен доступ к приватным полям.
copy_file("a.txt", "a-copy-2.txt");  // Не компилируется, ADL не помог.
std::filesystem::copy_file("a.txt", "a-copy-2.txt");  // Надо явно указать.
```
Почему строчка copy_file(from, to) компилируется? Потому что аргументы функции лежат в std::filesystem, поэтому мы ищем copy_file в нём же.  
Range-based-for и structured binding ищут `begin()`/`end()`/`get()` через ADL:
```c++
namespace ns {
    struct Foo { int x; };
    const int* begin(const Foo &f) { return &f.x; }
    const int* end(const Foo &f) { return &f.x + 1; }
};
int main() {
    ns::Foo f{20};
    for (int x : f) std::cout << x << '\n';
}
```
Причём _только_ через ADL: объявить `begin`/`end` глобально — ошибка.

---
## Примеры отключённого ADL
```c++
namespace foo {
    namespace impl {
        struct Foo { int x; };
        int func(const Foo &f) { return f.x; }
        int foo(const Foo &f) { return f.x; }
    }
    using impl::Foo;
}
namespace bar::impl {
    struct Bar{ int x; };
    int func(const Bar &f) { return f.x; }
}

int main() {
    foo::Foo f;
    bar::impl::Bar b;
    func(f);  // ok
    func(b);  // ok
    foo::impl::func(f);  // Qualified lookup, no ADL, ok.
    foo::impl::foo(f);  // Qualified lookup, no ADL, ok.
    foo::func(f);  // Qualified lookup, no ADL, compilation error.
    foo::foo(f);  // Qualified lookup, no ADL, compilation error.
    foo(f);  // compilation error: namespace foo
}
```

---
## `std::swap` и ADL
[Откуда взялся текущий `std::swap`](https://stackoverflow.com/a/5695855):

```c++
namespace std {
    template<typename T> void swap(T&, T&);
}
template<typename T> struct MyVec { .... };
namespace std {
    template<typename T> void swap<MyVec<T>>(...);  // Частичная специализация:(
    // В std:: можно только специализировать, но не перегружать.
    // template<typename T> void swap(MyVec<T>, MyVec<T>);  // Нельзя :(
}
```

* Плохой вариант: требовать от всех `a.swap(b)`: не работает с `int`.
* Вариант получше: ADL, и правильно использовать вот так:
  ```с++
  using std::swap;  // На случай, если стандартный через move подойдёт.
  swap(a, b);  // Вызываем с ADL.
  ```

Есть [ниблоиды](https://habr.com/ru/company/jugru/blog/447900/):
вызываем всегда через `std::swap`, который сам проверит `.swap`, ADL и стандартный.
Используется в C++20 для constrained algorithms.

---
## Тонкости ADL: hidden friends
```c++
#include <iostream>
namespace ns {
    struct Foo {
        friend void foo(Foo) {}  // implicitly inline.
    };
    // void foo(Foo);  // (1)
}
// ....
ns::Foo f;
foo(f);      // ok
ns::foo(f);  // compilation error
```
Если функция-друг определена внутри класса, то её можно найти __только__ через ADL

* Плюсы: функции можно вызвать только через ADL (примерно как методы), сложнее опечататься.
* Минусы: теперь эту функцию нельзя никуда явно передать, только лямбдой.

Можно раскомментировать `(1)` и функция станет видна в namespace.

---
## Прочие тонкости ADL
* Unqualified lookup смотрит на конструкторы, но ADL — нет:
    ```c++
    namespace ns {
        struct Foo {};
        struct Bar { Bar(Foo) {} };
        void foo() {
            Foo f;
            auto x = Bar(f);  // ok
        }
    }
    // ....
    ns::Foo f;
    auto x = Bar(f);  // compilation error
    ```
* ADL [иногда не может понять](https://stackoverflow.com/a/45493969/767632),
  что мы вызываем шаблон функции с явными шаблонными параметрами:
    ```c++
    std::tuple<int, int> a;
    std::get<0>(a);  // Ок.
    get<0>(a);       // (get < 0) > (a); ???
                     // В structured binding стоит костыль, чтобы работало.
    ```

---
## Возможный стиль: отключение ADL
До:
```c++
namespace ns {
    struct Foo { .... };
    struct Bar { .... };
    void func1(....);
    void func2(....);
    struct Baz { .... };
    void func_baz(Baz);
}
```

После (только пробелы надо нормальные):
```c++
namespace ns {
    namespace no_adl  { struct Foo { .... }; struct Bar { .... };   }
                        using no_adl::Foo;   using no_adl::Bar;
    void func1(....);
    void func2(....);
    namespace baz_adl { struct Baz { .... }; void func_baz(Baz);      }
                        using baz_adl::Baz;  using baz_adl::func_baz;
}
```

---
## Практические следствия ADL
* Вспомогательные для класса функции и операторы вроде `copy_file` должны быть в namespace класса
  ([1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#c5-place-helper-functions-in-the-same-namespace-as-the-class-they-support),
  [2](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#c168-define-overloaded-operators-in-the-namespace-of-their-operands)).
* Можно пользоваться ADL, но осторожно:
    * `count(vec.begin(), vec.end())` может не скомпилироваться
    * `vector::iterator` — это класс из `std`/`vector` или `typedef int*`?
* Осторожно с рефакторингами:
    * Перемещение функций, типов
* Если у вас вызывается странная функция без namespace — это ADL.
* Всегда пишите `using std::swap; swap(a, b)`.
