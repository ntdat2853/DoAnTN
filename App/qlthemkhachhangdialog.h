#ifndef QLTHEMKHACHHANGDIALOG_H
#define QLTHEMKHACHHANGDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class QLThemKhachHangDialog;
}

class QLThemKhachHangDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QLThemKhachHangDialog(QWidget *parent = nullptr);
    ~QLThemKhachHangDialog();
    void setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap);

signals:
    void khachHangAdded();

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleQuanLyInfoReply(QNetworkReply *reply);
    void handleCreateKhachHangReply(QNetworkReply *reply);

private:
    Ui::QLThemKhachHangDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    QString congTy; // Lưu CongTy của quản lý
};

#endif // QLTHEMKHACHHANGDIALOG_H
