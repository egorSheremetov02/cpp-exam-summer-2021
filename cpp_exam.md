# Билет 5. Многопоточность и сети

* Использование  `boost::asio::ip::tcp::iostream`  на сервере и на клиенте, отличия  `local_endpoint()`  от  `remote_endpoint()`.
  - `boost::asio::ip::tcp::iostream`- поток ввода/вывода, который можно получить как сервер, либо же как клиент, подключившись к какому-то серверу. Как это делается:
	  - Сервер:
		  ``` c++
		using boost::asio::ip::tcp;

		int main(int argc, char *argv[]) {
				assert(argc == 2); 
				boost::asio::io_context io_context;
				tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
				std::cout << "Listening at " << acceptor.local_endpoint() << "\n"; //отладочный вывод
				tcp::iostream client([&]() {
					tcp::socket s = acceptor.accept();
					std::cout << "Connected " << s.remote_endpoint() << " --> " << s.local_endpoint() << "\n";
					return s;
				}());
			
				while (client) {
					std::string s;
					client >> s;
					client << "string: " << s << "\n";
				}
				std::cout << "Completed\n";
				return 0;
			}
		```
		Обо всем по порядку.
		- `boost::asio::io_context io_context`- просто вспомогательная переменная, которая инициализирует системную библиотеку.
		- 	`tcp::acceptor acceptor` - с помощью этой переменной, сервер понимает где нужно слушать подключения клиентов. Параметр `tcp::v4()` - означает, что нужно слушать на всех ip-адресах, а `std::atoi(argv[1])` - обозначает номер порта (происходит явный каст от аски-строки в инт), на котором надо слушать.
		- `tcp::iostream client` - тут мы как раз вызывает конструктор нужного нам `tcp::iostream`, внутрь которого мы передаем лямбду. В ней мы вызываем `acceptor.accept()` - блокируется, и ждет нового подключения, после подключения возвращает сокет, соответствующий клиенту,  который мы и возвращаем из лямбды.
		- В цикле `while` (пока клиент подключен) мы просто используем `tcp::iostream` как обычный поток ввода/вывода.
	- Клиент:
		``` c++
		using boost::asio::ip::tcp;
		
		int main(int argc, char *argv[]) {
			assert(argc == 3);
			boost::asio::io_context io_context;
			auto create_connection = [&]() {
				tcp::socket s(io_context);
				boost::asio::connect(
				s,
				tcp::resolver(io_context).resolve(argv[1], argv[2]));
				return tcp::iostream(std::move(s));
			};
			tcp::iostream conn = create_connection();
			std::cout << "Connected " << conn.socket().local_endpoint() << " --> " << conn.socket().remote_endpoint() << "\n";
			
			conn << "hello world 123\n";
			conn.socket().shutdown(tcp::socket::shutdown_send);
			std::cout << "Shut down\n";
			
			int c;
			while ((c = conn.get()) != std::char_traits<char>::eof()) {
				std::cout << static_cast<char>(c);
			}
			std::cout << "Completed\n";
			return 0;
		}
		```
		Отметим отличия от сервера:
		- `tcp::resolver(io_context).resolve(argv[1], argv[2])` - подключает сокет к такому адресу и порту.
		- `conn.socket().shutdown(tcp::socket::shutdown_send);` - дает понять серверу, что больше мы ничего отправлять не будем.
		
  - `local_endpoint()` - порт, на котором установили соединение, а `remote_endpoint()` -  порт, с которым установили соединение.		
* Создание потоков, передача аргументов в функцию потока, joinable/detached потоки
	- Виды создания потоков:
		- `std::thread` с использованием лямбды:
			```C++
			std::thread t([&]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				std::cout << "Hello from thread! data=" << data << "\n";
				data += 10;
			});
			```
			Просто передаем лямбду в конструктор. `[&]` - означает, что передаем все видимые переменные по ссылке (то есть можем пользоваться ими внутри потока).
		- `std::thread` с явным указанием функции и ее параметров:
			```C++
			void worker(int a, int &b) {
				std::cout << "Thread: " << a << " " << &b << "\n";
			}
			.
			.
			.
			std::thread(worker, a, std::ref(b)).join();
			```
		Важно: всегда передаются копии параметров (если все-таки хотим передать ссылку, то надо явно писать std::ref() как в примее).
	- Если `main` закончился раньше, чем какой-нибудь поток, то, строго говоря, будут происходить утечки, что не есть хорошо, с этим помогает `t.join()` - программа ждем, пока поток завершится, и освобождает его ресурсы.
	- `t.detach()` - Обещаем больше не делать join(), теперь ОС отвечает за сборку мусора в потоке. Возникает только если нам вообще ничего от потока `t` никогда не будет нужно. Единственный разумный пример: мы создаём сетевой сервер, который в бесконечном цикле плодит потоки для клиентов.
* Гонки: при выводе на экран, по данным, одновременное чтение.
	- Вывод на экран:
		```C++
		void writeln(const char *s) {
			for (int i = 0; s[i]; i++) {
				std::cout << s[i];
			}
			std::cout << '\n';
		}
		
		int main() {
			std::thread t([]() {
				for (;;) {
					writeln("Hello from the second thread");
				}
			});
			for (;;) {
				writeln("Hello from the main thread");
			}
			return 0;
		}
		```
		Гонка происходит не смотря на потокобезопасность `std::cout`, посколько функции вызываются независимо, а значит оба цикла могут работать одновременно.
	- По данным:
		```C++
		int data = 0;
			auto worker = [&data]() {
				for (int i = 0; i < N; i++) {
				data++;
			}
		};
		std::thread t(worker);
		for (int i = 0; i < M; i++) {
			if (data % 2 == 0) {
				std::cout << "data is " << data << " (in progress)\n";
			}
		}
		t.join();
		std::cout << "data is " << data << "\n";
		```
		В данном случае может произойти так, что выведется нечетное значение. Так происходит, потому что между проверкой и выводом, внутри потока (который захватил `data` по ссылке - может произойти изменение).
