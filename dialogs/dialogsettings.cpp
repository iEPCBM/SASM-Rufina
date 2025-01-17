/*
 *  AMS Rufina - automated message delivery program
 *  Copyright (C) 2021 Rishat D. Kagirov (iEPCBM)
 *
 *     This file is part of AMS Rufina.
 *
 *  AMS Rufina is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "dialogsettings.h"
#include "ui_dialogsettings.h"

DialogSettings::DialogSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    settingsHandler = Settings::getInstance();
    QPushButton *btApply = ui->buttonBox->button (QDialogButtonBox::Apply);
    connect(btApply, SIGNAL(clicked()), this, SLOT(onApplied()));
    update();
}

DialogSettings::~DialogSettings()
{
    delete ui;
}

void DialogSettings::update()
{
    ui->leHSymbols->setText(settingsHandler->getHsymbols());
    ui->leSignature->setText(settingsHandler->getSignature());
    ui->chbUseKeyCry->setChecked(settingsHandler->isEncrypted());
    ui->btEditKey->setEnabled(settingsHandler->isEncrypted());
}

void DialogSettings::on_btEditChatsList_clicked()
{
    DialogChatsList d_chats(settingsHandler->getChats(),
                            settingsHandler->getVkToken(),
                            settingsHandler->isEncrypted(),
                            settingsHandler->getInitializeVector(), this);

    int resultDlg = d_chats.exec();
    if (resultDlg == QDialog::Accepted) {
        settingsHandler->setChats(d_chats.getChats());
    }
}

void DialogSettings::on_btShowToken_clicked()
{
    DialogToken dlgToken(this);
    if (settingsHandler->isEncrypted()) {
        DialogPasswordEnter dlgPswd(QByteArray::fromBase64(settingsHandler->getVkToken().toUtf8()),
                                    QByteArray::fromHex(settingsHandler->getInitializeVector().toUtf8()), this);
        dlgPswd.exec();
        if (dlgPswd.isSuccessful()) {
            dlgToken.setToken(dlgPswd.getDecryptedData());
            dlgToken.exec();
        }
    } else {
        dlgToken.setToken(settingsHandler->getVkToken());
        dlgToken.exec();
    }
}

bool DialogSettings::createPassword()
{
    DialogCreatePassword dlgCreatePswd(settingsHandler->getVkToken().toUtf8(), this);
    if (dlgCreatePswd.exec()==QDialog::Accepted) {
        settingsHandler->setVkToken(QString::fromUtf8(dlgCreatePswd.endcryptedData().toBase64()));
        settingsHandler->setInitializeVector(QString::fromUtf8(dlgCreatePswd.IV().toHex()));
        return true;
    }
    return false;
}

void DialogSettings::saveSettings()
{
    settingsHandler->setEncrypted(ui->chbUseKeyCry->isChecked());
    settingsHandler->setHsymbols(ui->leHSymbols->text());
    settingsHandler->setSignature(ui->leSignature->text());
    settingsHandler->save();
    emit saved(settingsHandler);
}

inline void DialogSettings::setEncryptedFlag(bool checked)
{
    settingsHandler->setEncrypted(checked);
    ui->chbUseKeyCry->setChecked(checked);
    ui->btEditKey->setEnabled(checked);
}

void DialogSettings::on_buttonBox_accepted()
{
    saveSettings();
    this->accept();
}

void DialogSettings::on_chbUseKeyCry_clicked(bool checked)
{
    if (checked) {
        setEncryptedFlag(createPassword());
    } else {
        if (QMessageBox::question(this,"Вы уверены?","Вы действительно хотите снять шифрование с токена?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes) {
            DialogPasswordEnter dlgPswd(QByteArray::fromBase64(settingsHandler->getVkToken().toUtf8()),
                                        QByteArray::fromHex(settingsHandler->getInitializeVector().toUtf8()), this);
            dlgPswd.exec();
            if (dlgPswd.isSuccessful()) {
                settingsHandler->setVkToken(dlgPswd.getDecryptedData());
                setEncryptedFlag(false);
            } else {
                setEncryptedFlag(true);
            }
        } else {
            setEncryptedFlag(true);
        }
    }
}

void DialogSettings::on_btEditKey_clicked()
{
    DialogPasswordEnter dlgPswd(QByteArray::fromBase64(settingsHandler->getVkToken().toUtf8()),
                                QByteArray::fromHex(settingsHandler->getInitializeVector().toUtf8()), this);
    dlgPswd.exec();
    if (dlgPswd.isSuccessful()) {
        DialogCreatePassword dlgCreatePswd(dlgPswd.getDecryptedData(), this);
        if (dlgCreatePswd.exec()==QDialog::Accepted) {
            settingsHandler->setVkToken(QString::fromUtf8(dlgCreatePswd.endcryptedData().toBase64()));
            settingsHandler->setInitializeVector(QString::fromUtf8(dlgCreatePswd.IV().toHex()));
        }
    }
}

void DialogSettings::on_btEditToken_clicked()
{
    QString token = "";
    if (settingsHandler->isEncrypted()) {
        DialogPasswordEnter dlgPswd(QByteArray::fromBase64(settingsHandler->getVkToken().toUtf8()),
                                    QByteArray::fromHex(settingsHandler->getInitializeVector().toUtf8()), this);
        dlgPswd.exec();
        if (dlgPswd.isSuccessful()) {
            token = QString::fromUtf8(dlgPswd.getDecryptedData());
            DialogEditToken dlgEdToken(token, this);
            dlgEdToken.exec();
            AESFacade aes(dlgEdToken.token().toUtf8());
            QByteArray encryData = aes.encryption(dlgPswd.getPassword());
            settingsHandler->setVkToken(QString::fromUtf8(encryData.toBase64()));
            settingsHandler->setInitializeVector(QString::fromUtf8(aes.getIV().toHex()));
        } else {
            return;
        }
    } else {
        token = settingsHandler->getVkToken();
        DialogEditToken dlgEdToken(token, this);
        dlgEdToken.exec();
        settingsHandler->setVkToken(dlgEdToken.token());
    }
}

void DialogSettings::on_btExportSettings_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Экспорт настроек", QDir::homePath(), "Файл настроек (*.xml);;Все файлы (*.*)");
    if (!filePath.isEmpty())
        settingsHandler->exportConf(filePath);
}

void DialogSettings::on_btImportSettings_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Импорт настроек", QDir::homePath(), "Файл настроек (*.xml);;Все файлы (*.*)");
    if (!filePath.isEmpty())
        settingsHandler->importConf(filePath);
    update();
}

void DialogSettings::onApplied()
{
    saveSettings();
}

void DialogSettings::on_buttonBox_rejected()
{
    this->reject();
}
