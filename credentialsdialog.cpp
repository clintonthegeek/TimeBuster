#include "credentialsdialog.h"
#include <QVBoxLayout>
#include <QLabel>

CredentialsDialog::CredentialsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("CalDAV Credentials");

    serverUrlEdit = new QLineEdit(this);
    usernameEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);

    serverUrlEdit->insert("https://my.opendesktop.org/remote.php/dav/calendars/");
    usernameEdit->insert("clintonthegeek");
    passwordEdit->insert("A9QAeL5GcrvAepkc");



    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Server URL (e.g., https://caldav.example.com):", this));
    layout->addWidget(serverUrlEdit);
    layout->addWidget(new QLabel("Username:", this));
    layout->addWidget(usernameEdit);
    layout->addWidget(new QLabel("Password:", this));
    layout->addWidget(passwordEdit);
    layout->addWidget(okButton);
    layout->addWidget(cancelButton);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString CredentialsDialog::serverUrl() const { return serverUrlEdit->text(); }
QString CredentialsDialog::username() const { return usernameEdit->text(); }
QString CredentialsDialog::password() const { return passwordEdit->text(); }
