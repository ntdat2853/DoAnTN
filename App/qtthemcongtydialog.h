#ifndef QTTHEMCONGTYDIALOG_H
#define QTTHEMCONGTYDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class QTThemCongTyDialog;
}

class QTThemCongTyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QTThemCongTyDialog(const QString &token, QWidget *parent = nullptr);
    ~QTThemCongTyDialog();

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleCreateQuanLyReply(QNetworkReply *reply);

private:
    Ui::QTThemCongTyDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
};

#endif // QTTHEMCONGTYDIALOG_H
