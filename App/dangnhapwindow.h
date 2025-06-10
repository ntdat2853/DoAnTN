#ifndef DANGNHAPWINDOW_H
#define DANGNHAPWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
namespace Ui {
class DangNhapWindow;
}
QT_END_NAMESPACE

class KhachHangWindow;
class QuanLyWindow;
class QuanTriWindow;

class DangNhapWindow : public QMainWindow
{
    Q_OBJECT

public:
    DangNhapWindow(QWidget *parent = nullptr);
    ~DangNhapWindow();

private slots:
    void on_pushButtonDangNhap_clicked();
    void on_pushButtonQuenMatKhau_clicked();
    void handleLoginReply(QNetworkReply *reply);
    void handleCheckTenDangNhapReply(QNetworkReply *reply);
    void handleGetUserInfoReply(QNetworkReply *reply);
    void handleUpdatePasswordReply(QNetworkReply *reply);
    void handleSendEmailReply(QNetworkReply *reply);

private:
    Ui::DangNhapWindow *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    QString vaiTro;
    QString newPassword;
    QString enteredGmail;
    KhachHangWindow *khachHangWindow;
    QuanLyWindow *quanLyWindow;
    QuanTriWindow *quanTriWindow;
    QString sendGridApiKey;
    bool isLoginProcessed; // Biến trạng thái đăng nhập

    QString generateRandomPassword();
    void sendEmail(const QString &toEmail, const QString &newPassword);
};

#endif // DANGNHAPWINDOW_H
