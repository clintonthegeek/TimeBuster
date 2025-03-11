#ifndef CREDENTIALSDIALOG_H
#define CREDENTIALSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class CredentialsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CredentialsDialog(QWidget *parent = nullptr);
    QString serverUrl() const;
    QString username() const;
    QString password() const;

private:
    QLineEdit *serverUrlEdit;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *okButton;
    QPushButton *cancelButton;
};

#endif // CREDENTIALSDIALOG_H
