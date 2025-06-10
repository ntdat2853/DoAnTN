#include "dangnhapwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DangNhapWindow *w = new DangNhapWindow();
    w->setAttribute(Qt::WA_QuitOnClose, false); // Ngăn ứng dụng thoát khi đóng cửa sổ
    w->show();
    return a.exec();
}
