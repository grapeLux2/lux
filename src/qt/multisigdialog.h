#ifndef MULTISIGDIALOG_H
#define MULTISIGDIALOG_H

#include "../src/key.h"
#include <QDialog>
#include <QFrame>
#include <QString>
#include <QVBoxLayout>
#include "script/script.h"
#include "primitives/transaction.h"
#include "coins.h"
#include "coincontrol.h"
#include "walletmodel.h"
#include "coincontroldialog.h"
#include "rpcserver.h"
#include "coincontroldialog.h"


namespace Ui
{
class MultisigDialog;
}

class MultisigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MultisigDialog(QWidget* parent);
    ~MultisigDialog();
    void setModel(WalletModel* model);
    //void updateCoinControl(CAmount nAmount, unsigned int nQuantity);


private:
    Ui::MultisigDialog* ui;
    WalletModel* model;
    CMutableTransaction multisigTx;

private slots:
//////////////////////////////////////////// generate multi sig addr
void add_pubkey_make_multi_sig_addr();
void createmultisigGUI();
//void remove_selected_addr();
void clear_addr();
//////////////////////////////////////////// generate multisig raw TX 
void add_input_make_multi_sig_TX();
void add_output_make_multi_sig_TX();
void createrawtransactionGUI();
void reset_make_tx();
void clear_TX_in();
void clear_TX_out();
///////////////////////////////////////////sign raw TX
void signtransactionGUI();
void sendtransactionGUI();
void clear_sign();
    
//////////////////////////////////////////// sign multi sig TX
//void addrToPubKey TODO
};

#endif // BITCOIN_QT_MULTISIGDIALOG_H
