#include "main.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qSetMessagePattern("%{time [yyyy.MM.dd]} %{time [hh:mm:ss]} %{type} ====> %{message} ");

    MainWindow w;
    w.show();
    return a.exec();
}
