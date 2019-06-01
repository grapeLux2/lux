// Copyright (c) 2015-2019 The LUX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "key.h"
#include "../src/core_io.h"
#include "../src//univalue/univalue.h"
#include "../src/rpcutil.h"
#include "multisigdialog.h"
#include "forms/ui_multisigdialog.h"
#include "../src/rpcserver.h"
#include "pubkey.h"
#include "uint256.h"
#include <QDoubleSpinBox>
#include <QtCore/QVariant>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QClipboard>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QString>
#include <QDir>
#include <QTextStream>

CPubKey pubkey;
CKey Schnorr;

MultisigDialog::MultisigDialog(QWidget* parent) : QDialog(parent),
                                                  ui(new Ui::MultisigDialog),
                                                  model(0)
{
/* Since we are dealing with a ui that has 3 "parts",
I have sectioned each part to make it easyer to understand 
where everything is same with the header file*/

    ui->setupUi(this);
///////////////////////////////////////////////////////////////////////////make multi sig addr
    ui->create_addr->setEnabled(false);
    ui->numsigs->setRange(2, 15);// limit the min number of sigs to 2 and max to 15
    connect(ui->makeaddr_ADD_PUB_KEY, SIGNAL(clicked()), this, SLOT(add_pubkey_make_multi_sig_addr())); // adds the pub key of an addres to the multisig addr tabel
    connect(ui->add_output, SIGNAL(clicked()), this, SLOT(add_output_make_multi_sig_TX())); // adds an output
    connect(ui->create_addr, SIGNAL(clicked()), this, SLOT(createmultisigGUI())); // creates the multisig addr
    connect(ui->reset_addr, SIGNAL(clicked()), this, SLOT(clear_addr()));
///////////////////////////////////////////////////////////////////////////make multi sig TX
    connect(ui->send, SIGNAL(clicked()), this, SLOT(createrawtransactionGUI()));// creates the raw TX
    connect(ui->addinput, SIGNAL(clicked()), this, SLOT(add_input_make_multi_sig_TX())); // adds an input (in the hex form) and Vout to the multi sig raw tx generation function
    connect(ui->clear_in, SIGNAL(clicked()), this, SLOT(clear_TX_in()));
    connect(ui->clear_out, SIGNAL(clicked()), this, SLOT(clear_TX_out()));
    connect(ui->reset, SIGNAL(clicked()), this, SLOT(reset()));

///////////////////////////////////////////////////////////////////////////sign + send multi sig TX
    ui->send_TX->setEnabled(false); // start with the send TX button disabled
    connect(ui->sign, SIGNAL(clicked()), this, SLOT(signtransactionGUI()));
    connect(ui->sig_reset, SIGNAL(clicked()), this, SLOT(clear_sign()));
    connect(ui->send_TX, SIGNAL(clicked()), this, SLOT(sendtransactionGUI()));

}

MultisigDialog::~MultisigDialog()
{
    delete ui;
} 

void MultisigDialog::setModel(WalletModel *model)
{
    this->model = model;
}
 
void MultisigDialog::add_pubkey_make_multi_sig_addr() // add the pubkey
{
    ui->create_addr->setEnabled(false);
    int num = 0;
    for (int i = 0; i < ui->treeWidget_addr->topLevelItemCount(); i++){ // find how many rows we have 
       num++; 
    }


    if (num <= 14){
        QTreeWidgetItem *treeItem = new QTreeWidgetItem(ui->treeWidget_addr);
        treeItem->setText(0, QVariant(num).toString()); // change our int value to a Qstring and display
        treeItem->setText(1, ui->addpubkey->text()); // display the pubkey of the address
    }else {
        ui->makeaddrRS->setText(tr("ERROR!, the max number of address per multisig address is 15"));
    } 
    num++;
    if (num > 1){
        ui->create_addr->setEnabled(true);
    }        
 
}

void MultisigDialog::add_input_make_multi_sig_TX() // add the pubkey
{
    int num = 0;
        for (int i = 0; i < ui->tree_wiget_create_raw->topLevelItemCount(); i++) // find how many rows we have 
        {
           num++; 
        }
            QTreeWidgetItem *treeItem_TX = new QTreeWidgetItem(ui->tree_wiget_create_raw);
            treeItem_TX->setText(0, QVariant(num).toString()); // change our int value to a Qstring and display
            treeItem_TX->setText(1, ui->V_OUT->text()); // display the input
            treeItem_TX->setText(2, ui->addscriptkey->text()); // display the input
            ui->addOutput->clear();
            ui->addOutput->setText("script key of input in the HEX form");
            ui->V_OUT->clear();
            ui->V_OUT->setText("V OUT");

}

void MultisigDialog::add_output_make_multi_sig_TX() // add the output
{
int num = 0;
        for (int i = 0; i < ui->tree_wiget_create_raw_output->topLevelItemCount(); i++) // find how many rows we have 
        {
           num++; 
        }
            QTreeWidgetItem *treeItem_TX_output = new QTreeWidgetItem(ui->tree_wiget_create_raw_output);
            treeItem_TX_output->setText(0, QVariant(num).toString()); // change our int value to a Qstring and display
            treeItem_TX_output->setText(1, ui->amount->text()); // display the input
            treeItem_TX_output->setText(2, ui->addOutput->text()); // display the input
            ui->addscriptkey->clear();
            ui->addscriptkey->setText("add Output address");
            ui->amount->clear();
            ui->amount->setText("amount");
}

void MultisigDialog::createmultisigGUI()
{
    int num = 0;
    for (int i = 0; i < ui->treeWidget_addr->topLevelItemCount(); i++){ // find how many pubkeys we have 
       num++; 
    }

    int required = ui->numsigs->value(); // how many sigs are required to vailadte a tx
    if (required > num){// check that min number of sigs required to validate a tx is not more than the number of address that can sign the tx
        ui->makeaddrRS->setText(tr("ERROR! the minimum number of signatures is more than the number of address that can sign a transaction")); 
        return;
    }

    std::vector<CPubKey> pubkeys;
    if(num > 1){ // check that we have atleast 2 pubkeys 
        for (int i = 0; i < ui->treeWidget_addr->topLevelItemCount(); ++i){ 



            QString keyst = ui->treeWidget_addr->topLevelItem(i)->text(1); // read the current row at colum 1 (pukey) and store as keyst
            QByteArray str = keyst.toLocal8Bit(); // convert from Qstring to local 8 bit
            const UniValue key = str.data(); // convert from local 8 bit to a UniValue compatible value

            if (IsHex(key.get_str()) && (key.get_str().length() == 66 || key.get_str().length() == 130)) { // check pubkey is vaild 
                pubkeys.push_back(HexToPubKey(key.get_str())); 
            }else{
            ui->makeaddrRS->setText(tr(" failed!, please check pubkeys"));   
            return;  
            }
        }

    }else{
            ui->makeaddrRS->setText(tr("failed!, 2 or more valid pubkeys are needed!"));
            return;
    }

    // Construct using pay-to-script-hash:
    CScript inner = CreateMultisigRedeemscript(required, pubkeys);
    CScriptID innerID(inner);

    ui->makeaddress->setText(EncodeDestination(innerID).c_str()); // return the multi sig address 
    ui->makeaddrRS->setText(HexStr(inner.begin(), inner.end()).c_str()); // return the reddem script 

}

/////////////////////////////////////////////////////////////////////////////////////////////////multi sig raw tx

void MultisigDialog::createrawtransactionGUI() 
{

    
//this here is very important we make it clear to the user they need a chage address
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "ARE YOU SURE?", "STOP!!!, READ THIS CARFULY, you are about to send a transaction DID YOU MAKE SURE TO ADD A CHANGE ADDRESS, WITH THE CORRECT AMOUNT?", QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) {
        QMessageBox msgBox;
        msgBox.setObjectName("multi sig");
        msgBox.setText(tr("Cancled"));
        msgBox.exec();
        return;
    }
    

    CMutableTransaction rawTx;
    for (int i = 0; i < ui->tree_wiget_create_raw->topLevelItemCount(); i++){
        uint256 txid = uint256S(ui->tree_wiget_create_raw->topLevelItem(i)->text(2).toStdString());
        int nOutput = ui->tree_wiget_create_raw->topLevelItem(i)->text(1).toInt();
        CTxIn in(COutPoint(txid, nOutput));
        rawTx.vin.push_back(in);   
    }

    set<CTxDestination > destinations;

    for (int i = 0; i < ui->tree_wiget_create_raw_output->topLevelItemCount(); i++) {

        if (ui->tree_wiget_create_raw_output->topLevelItem(i)->text(2).isEmpty()){ 
        ui->makerawtx->setText("no address to send to"); 
            return;
        }
        QString keyst = ui->tree_wiget_create_raw_output->topLevelItem(i)->text(2); // read the current row at colum 1 (pukey) and store as keyst
        QByteArray str = keyst.toLocal8Bit(); // convert from Qstring to local 8 bit
        const string& name_  = str.toStdString();
        CTxDestination destination = DecodeDestination(name_);

        if (!IsValidDestination(destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Lux address: ")+name_);
        }

        if (!destinations.insert(destination).second) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ") + name_);
        }

        CScript scriptPubKey = GetScriptForDestination(destination);
        CAmount nAmount = ui->tree_wiget_create_raw_output->topLevelItem(i)->text(1).toInt();
        CTxOut out(nAmount, scriptPubKey);
        rawTx.vout.push_back(out);
          
    }

    ui->makerawtx->setText(EncodeHexTx(rawTx).c_str());
}

void MultisigDialog::signtransactionGUI(){
    uint256 hash = uint256S(ui->hash->text().toStdString());
    QString VC = ui->PK->text().toStdString();
    //VCH = VC.GetPubKey();
    //std::vector<uint8_t> vchSig(ParseHex(ui->PK->text().toStdString()));
    std::vector<uint8_t> vchSig(GetPubKey(VC));   
    if (Schnorr.SignSchnorr(hash, vchSig)){
    //if (pubkey.VerifySchnorr(hash, vchSig)){
        ui->sign_hex->setText("signed");
        return;
    } else{ 
        ui->sign_hex->setText("failed");
        return;
    }
    ui->sign_hex->setText("exited");
}

void MultisigDialog::sendtransactionGUI()
{/*
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "sendrawtransaction \"hexstring\" ( allowhighfees )\n"
            "\nSubmits raw transaction (serialized, hex-encoded) to local node and network.\n"
            "\nAlso see createrawtransaction and signrawtransaction calls.\n"
            "\nArguments:\n"
            "1. \"hexstring\"    (string, required) The hex string of the raw transaction)\n"
            "2. allowhighfees    (boolean, optional, default=false) Allow high fees\n"
            "\nResult:\n"
            "\"hex\"             (string) The transaction hash in hex\n"
            "\nExamples:\n"
            "\nCreate a transaction\n" +
            HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n" + HelpExampleCli("signrawtransaction", "\"myhex\"") +
            "\nSend the transaction (signed hex)\n" + HelpExampleCli("sendrawtransaction", "\"signedhex\"") +
            "\nAs a json rpc call\n" + HelpExampleRpc("sendrawtransaction", "\"signedhex\""));

    LOCK(cs_main);

    RPCTypeCheck(params, list_of(UniValue::VSTR)(UniValue::VBOOL));

    // parse hex string from parameter
    CTransaction tx;
    if (!DecodeHexTx(tx, params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    uint256 txid = uint256S(ui->txid->text().toStdString());
    uint256 hashTx = tx.GetHash();
    uint256 txid = uint256S(ui->txid->text().toStdString());

    bool fOverrideFees = false;
    if (params.size() > 1)
        fOverrideFees = params[1].get_bool();

    CCoinsViewCache& view = *pcoinsTip;
    const CCoins* existingCoins = view.AccessCoins(hashTx);
    bool fHaveMempool = mempool.exists(hashTx);
    bool fHaveChain = existingCoins && existingCoins->nHeight < 1000000000;
    if (!fHaveMempool && !fHaveChain) {
        // push to local node and sync with wallets
        CValidationState state;
        bool fMissingInputs;
        if (!AcceptToMemoryPool(mempool, state, tx, false, &fMissingInputs, nullptr, !fOverrideFees)) {
            if (state.IsInvalid()) {
                throw JSONRPCError(RPC_TRANSACTION_REJECTED,
                                   strprintf("%i: %s", state.GetRejectCode(), state.GetRejectReason()));
            } else {
                if (fMissingInputs) {
                    throw JSONRPCError(RPC_TRANSACTION_ERROR, "Missing inputs");
                }
                throw JSONRPCError(RPC_TRANSACTION_ERROR, state.GetRejectReason());
            }
        }
    } else if (fHaveChain) {
        throw JSONRPCError(RPC_TRANSACTION_ALREADY_IN_CHAIN, "transaction already in block chain");
    }
    RelayTransaction(tx);

    return hashTx.GetHex();
*/}

void MultisigDialog::clear_addr()
{
    ui->makeaddress->clear();
    ui->makeaddrRS->clear();
    ui->treeWidget_addr->clear();
}

void MultisigDialog::reset_make_tx()
{
    ui->addOutput->clear();
    ui->addOutput->setText("script key of input in the HEX form");
    ui->V_OUT->clear();
    ui->V_OUT->setText("V OUT");
    ui->addscriptkey->clear();
    ui->addscriptkey->setText("add Output address");
    ui->amount->clear();
    ui->amount->setText("amount");
    ui->makerawtx->clear();
    clear_TX_in();
    clear_TX_out();
}

void MultisigDialog::clear_TX_in(){
ui->tree_wiget_create_raw->clear();
}

void MultisigDialog::clear_TX_out(){
ui->tree_wiget_create_raw_output->clear();
}

void MultisigDialog::clear_sign()// sign and send
{
    ui->hash->setText("hash");
    ui->hash->clear();
    ui->PK->clear();
    ui->PK->setText("Private key");
    ui->sign_hex->clear();
}
