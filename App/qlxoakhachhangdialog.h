#ifndef QLXOAKHACHHANGDIALOG_H
#define QLXOAKHACHHANGDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class QLXoaKhachHangDialog;
}

class QLXoaKhachHangDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QLXoaKhachHangDialog(QWidget *parent = nullptr);
    ~QLXoaKhachHangDialog();

    void setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap, const QString &congTy);

signals:
    void customerDeleted();

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleKhachHangReply(QNetworkReply *reply);
    void handleDeleteReply(QNetworkReply *reply);

private:
    Ui::QLXoaKhachHangDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    QString congTy;
    QList<QPair<int, int>> pendingDeletions; // Lưu danh sách IDTaiKhoan và IDKhachHang cần xóa
    int deletionIndex; // Theo dõi tiến trình xóa

    void loadKhachHangList();
};

#endif // QLXOAKHACHHANGDIALOG_H
