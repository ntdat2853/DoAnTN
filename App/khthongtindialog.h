#ifndef KHTHONGTINDIALOG_H
#define KHTHONGTINDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class KHThongTinDialog;
}

class KHThongTinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KHThongTinDialog(QWidget *parent = nullptr);
    ~KHThongTinDialog();
    void setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap);

signals:
    void infoUpdated(const QString &hoVaTen); // Signal báo hiệu cập nhật thông tin

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleKhachHangInfoReply(QNetworkReply *reply);
    void handleUpdateInfoReply(QNetworkReply *reply);

private:
    Ui::KHThongTinDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    QString congTy; // Lưu CongTy để giữ nguyên
};

#endif // KHTHONGTINDIALOG_H
