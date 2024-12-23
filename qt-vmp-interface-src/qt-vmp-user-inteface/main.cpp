#include "main.h"

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{time [yyyy.MM.dd]} %{time [hh:mm:ss]} %{type} ====> %{message} ");

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
