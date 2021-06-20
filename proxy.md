## Proxy && vector<bool>

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
