#ifndef QUANTRIWINDOW_H
#define QUANTRIWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace Ui {
class QuanTriWindow;
}

class DangNhapWindow;

class QuanTriWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit QuanTriWindow(const QString &token, const QString &tenDangNhap, QWidget *parent = nullptr);
    ~QuanTriWindow();

private slots:
    void on_pushButtonDangXuat_clicked();
    void on_pushButtonThemCongTy_clicked();
    void on_pushButtonXoaCongTy_clicked();
    void on_pushButtonDoiMatKhau_clicked();
    void on_pushButtonThongTinQuanTri_clicked();
    void handleQuanLyReply(QNetworkReply *reply);
    void handleQuanTriInfoReply(QNetworkReply *reply);
    void onDetailButtonClicked(int row);
    void handleTaiKhoanReply(QNetworkReply *reply); // Slot mới để xử lý phản hồi từ /taikhoan/{ten_dang_nhap}

private:
    Ui::QuanTriWindow *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    DangNhapWindow *dangNhapWindow;
    bool isLogoutProcessed; // Biến trạng thái đăng xuất
    void showQuanLyDetails(const QJsonObject &taikhoan); // Hiển thị thông tin chi tiết quản lý
};

#endif // QUANTRIWINDOW_H
