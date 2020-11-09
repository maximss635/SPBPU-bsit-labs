#include "Client.h"

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Incorrect arguments\n");
        return -1;
    }

    Client client(argv[1]);
    if (client.try_connect()) {
        printf("Success connection to %s:%d\n", client.get_ip(), client.get_port());
    }
    else {
        printf("No connection...\n");
        return -1;
    }

    // формируем запос чтобы отправить ключ
    client.request_formation();

    // отправляем запрос
    client.send_request();

    // ждем ответа на отправленный ключ
    printf("Wait answer...\n");
    client.recv_request();

    // На основе ответа устанавливаем ключ для дальнейшей переписки
    client.set_key();


    assert (client.crypt_test(client.get_key()));

    while (true) {
        client.print_dialog();
        if (client.ask_request_type()) {
            return 0;
        }

        client.request_formation();
        client.send_request();

        // ждем ответа на отправленное сообщение
        printf("Wait answer...\n");
        client.recv_request();

        // Расшифровывваем и выводим на экран полученное сообщение
        client.decrypt_msg();

        getchar();
        getchar();
        system("cls");

        client.clear_buf();
    }
}
