#include "CTabMain.h"
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QComboBox>
#include <QToolTip>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

CTabMain::CTabMain(QWidget *parent) : QTabWidget(parent)
{
    InitTabMain();
}

void CTabMain::InitTabMain(){
    m_pXML = new XML();
    if(m_pXML->getConfigureInfo(":/config")){
        QList<Rader_Total>::iterator it = m_pXML->mRaderTotals.begin();
        for( ;it!= m_pXML->mRaderTotals.end();it++ ){
            //Tab Tag
            Rader_Total raderTotal = *it;
            //qDebug()<<"A\t"<<raderTotal.displayName<<raderTotal.subjects.size();
            QTreeWidget* pCurTree = new QTreeWidget(this);
            pCurTree->setHeaderHidden(true);
            pCurTree->setColumnCount(3);
            pCurTree->setColumnWidth(0,300);
            this->addTab(pCurTree,raderTotal.displayName);
            //保存到队列中
            m_treeWidgetList.push_back(pCurTree);
            m_pCurTree = pCurTree;

            //Subject
            QList<Subject> subjects = raderTotal.subjects;
            QList<Subject>::iterator itSubject = subjects.begin();
            for( ;itSubject!=subjects.end();itSubject++ ){
                Subject subject = *itSubject;
                //qDebug()<<"B\t\t"<<subject.displayName;
                QTreeWidgetItem* pSubject = new QTreeWidgetItem(pCurTree);
                pSubject->setText(0,subject.displayName);

                //SubjectInfo
                QList<SubjectInfo> subjectInfos = subject.subjectInfos;
                QList<SubjectInfo>::iterator itSubInfo = subjectInfos.begin();
                for( ;itSubInfo != subjectInfos.end(); itSubInfo++){
                    SubjectInfo subjectInfo = *itSubInfo;
                    //qDebug()<<"C\t\t\t"<<subjectInfo.displayName<<subjectInfo.ents.size();
                    QTreeWidgetItem* pSubInfo = new QTreeWidgetItem(pSubject);
                    pSubInfo->setText(0,subjectInfo.displayName);

                    //Entity
                    QList<Entity> ents = subjectInfo.ents;
                    QList<Entity>::iterator itEnts = ents.begin();
                    for( ;itEnts!=ents.end();itEnts++ ){
                        Entity ent = *itEnts;
                        //qDebug()<<"D\t\t\t\t"<<ent.displayName;
                        QTreeWidgetItem* pEnt = new QTreeWidgetItem(pSubInfo);
                        pEnt->setText(0,ent.displayName);

                        //Attr
                        QList<Attr> attrs = ent.attrs;
                        QList<Attr>::iterator itAttr = attrs.begin();
                        for( ;itAttr!=attrs.end(); ){
                            Attr attr = *itAttr;
                            //qDebug()<<"E\t\t\t\t\t"<<attr.displayName;
                            QTreeWidgetItem* pAttr = new QTreeWidgetItem(pEnt);

                            QLabel* pLable = new QLabel(attr.displayName);
                            pLable->setObjectName("Lable");
                            pCurTree->setItemWidget(pAttr,0,pLable);
                            if( "control" == subjectInfo.type ){
                                if( "text" == attr.displayType ){
                                    QLineEdit* pLineEdit = new QLineEdit();
                                    pLineEdit->setObjectName("LineEdit");
                                    pLineEdit->setFrame(true);
                                    pLineEdit->setMaximumWidth(100);
                                    pLineEdit->setText(attr.value);
                                    pCurTree->setItemWidget(pAttr,1,pLineEdit);
                                    connect(pLineEdit,SIGNAL(textEdited(QString)),this,SLOT(lineTextEdited(QString)));
                                    QStringList strValidTips;
                                    strValidTips<<attr.validator<<attr.tips;
                                    m_lineEditTips.insert(pLineEdit,strValidTips);
                                }
                                else if("select"==attr.displayType){
                                    QComboBox* pCombo = new QComboBox();
                                    pCombo->setObjectName("ComboBox");
                                    pCombo->setFrame(true);
                                    pCombo->setMaximumWidth(100);
                                    pCombo->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
                                    QStringList paramTypes = attr.paramType.split(";");
                                    pCombo->addItems(paramTypes);
                                    pCurTree->setItemWidget(pAttr,1,pCombo);
                                }
                                itAttr++;
                                if( itAttr==attrs.end() ){
                                    QPushButton* pBtn = new QPushButton("设置",this);
                                    pBtn->setMaximumWidth(100);
                                    connect(pBtn,SIGNAL(clicked(bool)),this,SLOT(btnSetClicked()));
                                    pCurTree->setItemWidget(pAttr,2,pBtn);
                                    //将设置按钮与父Item关联
                                    m_btnMap.insert(pBtn,pEnt);
                                }
                            }
                            if( "monitor" == subjectInfo.type ){
                                pAttr->setText(1,attr.value);
                                itAttr++;
                            }
                        }
                    }
                }
            }
            pCurTree->expandAll();
        }
    }
    connect(this,SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));
}

void CTabMain::currentTabChanged(int index){
    m_pCurTree = m_treeWidgetList.at(index);
}

void CTabMain::lineTextEdited(QString strText){
    QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(sender());
    QMap<QLineEdit*,QStringList>::iterator it = m_lineEditTips.find(pLineEdit);
    if( it != m_lineEditTips.end() ){
        //QRegExp regx("[0-9]+$");
        QStringList strValidTips = it.value();
        QString strValid = strValidTips.at(0);
        QString strTips = strValidTips.at(1);
        QRegExp regx(strValid);
        QValidator *validator = new QRegExpValidator(regx, 0);
        int nPos = 0;
        if( QValidator::Acceptable != validator->validate(strText,nPos) ){
            // 错误提示
            QToolTip::showText(pLineEdit->mapToGlobal(QPoint(100, 0)),strTips);
            pLineEdit->clear();
        }
    }
}

void CTabMain::btnSetClicked(){
    QPushButton* pBtn = qobject_cast<QPushButton*>(sender());
    QMap<QPushButton*,QTreeWidgetItem*>::iterator it = m_btnMap.find(pBtn);
    if( it != m_btnMap.end() ){
        QTreeWidgetItem* pEntItem = it.value();
        if( NULL != pEntItem ){
            int nAttrs = pEntItem->childCount();
            for( int i=0;i<nAttrs;i++ ){
                QTreeWidgetItem* pAttrItem = pEntItem->child(i);
                QWidget* pAttr = m_pCurTree->itemWidget(pAttrItem,0);
                if( "Lable" == pAttr->objectName() ){
                    QLabel* pLabel = (QLabel*)pAttr;
                    qDebug()<<pLabel->text();
                }
                QWidget* pValue = m_pCurTree->itemWidget(pAttrItem,1);
                if( "LineEdit" == pValue->objectName() ){
                    QLineEdit* pLineEdit = (QLineEdit*)pValue;
                    qDebug()<<pLineEdit->text();
                }
                else if( "ComboBox" == pValue->objectName() ){
                    QComboBox* pComboBox = (QComboBox*)pValue;
                    qDebug()<<pComboBox->currentText();
                }
            }
        }
    }
}
