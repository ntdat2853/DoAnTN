#ifndef QUANLYWINDOW_H
#define QUANLYWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class QuanLyWindow;
}

class DangNhapWindow;

class QuanLyWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit QuanLyWindow(const QString &token, const QString &tenDangNhap, QWidget *parent = nullptr);
    ~QuanLyWindow();

private slots:
    void on_pushButtonDangXuat_clicked();
    void on_pushButtonThemKhachHang_clicked();
    void on_pushButtonXoaKhachHang_clicked();
    void on_pushButtonNhapGia_clicked();
    void on_pushButtonDoiMatKhau_clicked();
    void on_pushButtonThongTinQuanLy_clicked();
    void handleGiaMuReply(QNetworkReply *reply);
    void handleKhachHangReply(QNetworkReply *reply);
    void handleQuanLyInfoReply(QNetworkReply *reply);
    void onDetailButtonClicked(int row); // Slot xử lý khi nhấn nút Chi tiết
    void updateManagerName(const QString &hoVaTen); // Slot để cập nhật tên
    void refreshInterface(); // Slot để làm mới giao diện

private:
    Ui::QuanLyWindow *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    QString congTy; // Lưu CongTy của QuanLy
    DangNhapWindow *dangNhapWindow;
    bool isLogoutProcessed; // Biến trạng thái đăng xuất
    void showKhachHangDetails(const QJsonObject &taikhoan); // Hiển thị thông tin chi tiết khách hàng
};

#endif // QUANLYWINDOW_H
