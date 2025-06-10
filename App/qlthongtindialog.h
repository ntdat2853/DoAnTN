#ifndef QLTHONGTINDIALOG_H
#define QLTHONGTINDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class QLThongTinDialog;
}

class QLThongTinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QLThongTinDialog(QWidget *parent = nullptr);
    ~QLThongTinDialog();
    void setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap);

signals:
    void infoUpdated(const QString &hoVaTen); // Signal báo hiệu cập nhật thông tin

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleQuanLyInfoReply(QNetworkReply *reply);
    void handleUpdateInfoReply(QNetworkReply *reply);

private:
    Ui::QLThongTinDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
};

#endif // QLTHONGTINDIALOG_H
