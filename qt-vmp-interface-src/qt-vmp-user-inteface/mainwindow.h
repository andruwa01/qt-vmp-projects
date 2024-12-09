#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QIntValidator>
#include <QRegularExpressionValidator>

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow *ui;

    void setValidationIp();
    void setValidationPort();
    void setValidationFreq();
    void setActionOnButtonClicked();

    void checkInputQLine(const QString &text);
    void onPushButtonClicked();
};
#endif // MAINWINDOW_H
