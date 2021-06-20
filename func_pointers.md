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

