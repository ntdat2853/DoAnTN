#ifndef KHACHHANGWINDOW_H
#define KHACHHANGWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QJsonArray>

namespace Ui {
class KhachHangWindow;
}

class DangNhapWindow;

class KhachHangWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit KhachHangWindow(const QString &token, const QString &tenDangNhap, QWidget *parent = nullptr);
    ~KhachHangWindow();

private slots:
    void on_pushButtonDangXuat_clicked();
    void on_pushButtonDoiMatKhau_clicked();
    void on_pushButtonThongTinKhachHang_clicked();
    void on_chiTietButton_clicked(int row);  // Slot mới cho nút Chi tiết
    void handleKhachHangInfoReply(QNetworkReply *reply);
    void handleGiaoDichReply(QNetworkReply *reply);
    void handleGiaMuReply(QNetworkReply *reply);
    void handleThanhToanReply(QNetworkReply *reply);
    void updateCustomerName(const QString &hoVaTen);

private:
    Ui::KhachHangWindow *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    int idKhachHang;
    QString congTy;
    QJsonArray giaoDichList;  // Lưu trữ danh sách giao dịch
    DangNhapWindow *dangNhapWindow;
    bool isLogoutProcessed; // Biến trạng thái đăng xuất
    bool isThongTinProcessed;
    bool isDMKProcessed;
};

#endif // KHACHHANGWINDOW_H
