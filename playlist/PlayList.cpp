﻿#include "PlayList.h"
#include "PlayListModel.h"
#include "PlayListDelegate.h"
#include <QFileDialog>
#include <QListView>
#include <QLayout>
#include <QToolButton>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QtDebug>

PlayList::PlayList(QWidget *parent) :
    QWidget(parent)
{
    mFirstShow = true;
    mMaxRows = -1;
    mpModel = new PlayListModel(this);
    mpDelegate = new PlayListDelegate(this);
    mpListView = new QListView;
    //mpListView->setResizeMode(QListView::Adjust);
    mpListView->setModel(mpModel);
    mpListView->setItemDelegate(mpDelegate);
    mpListView->setSelectionMode(QAbstractItemView::ExtendedSelection); //ctrl,shift
    mpListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mpListView->setToolTip(QString::fromLatin1("Ctrl/Shift + ") + tr("Click to select multiple"));
    QVBoxLayout *vbl = new QVBoxLayout;
    setLayout(vbl);
    vbl->addWidget(mpListView);
    QHBoxLayout *hbl = new QHBoxLayout;

    mpClear = new QToolButton(0);
    mpClear->setText(tr("Clear"));
    mpRemove = new QToolButton(0);
    mpRemove->setText(QString::fromLatin1("-"));
    mpRemove->setToolTip(tr("Remove selected items"));
    mpAdd = new QToolButton(0);
    mpAdd->setText(QString::fromLatin1("+"));

    hbl->addWidget(mpClear);
    hbl->addSpacing(width());
    hbl->addWidget(mpRemove);
    hbl->addWidget(mpAdd);
    vbl->addLayout(hbl);
    connect(mpClear, SIGNAL(clicked()), SLOT(clearItems()));
    connect(mpRemove, SIGNAL(clicked()), SLOT(removeSelectedItems()));
    connect(mpAdd, SIGNAL(clicked()), SLOT(addItems()));
    connect(mpListView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onAboutToPlay(QModelIndex)));
    // enter to highight
    //connect(mpListView, SIGNAL(entered(QModelIndex)), SLOT(highlight(QModelIndex)));
}

PlayList::~PlayList()
{
    qDebug("+++++++++++++~PlayList()");
    save();
}

void PlayList::setSaveFile(const QString &file)
{
    mFile = file;
}

QString PlayList::saveFile() const
{
    return mFile;
}

void PlayList::load()
{
    QFile f(mFile);
    if (!f.exists())
        return;
    if (!f.open(QIODevice::ReadOnly))
        return;
    QDataStream ds(&f);
    QList<PlayListItem> list;
    ds >> list;
    for (int i = 0; i < list.size(); ++i) {
        insertItemAt(list.at(i), i);
    }
}

void PlayList::save()
{
    QFile f(mFile);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning("File open error: %s", qPrintable(f.errorString()));
        return;
    }
    QDataStream ds(&f);
    ds << mpModel->items();
}

void PlayList::deleteSaveFile()
{
    QFileInfo fi(mFile);
    if (fi.exists() && fi.isFile())
    {
        QFile f(mFile);
        f.remove(mFile);
        qDebug() << "[PlayList/History] : "
                 << mFile.toUtf8().constData()
                 << "has been deleted.";
    }
    else
    {
        qDebug() << "[PlayList/History] : "
                 << mFile.toUtf8().constData()
                 << "does not exist.";
    }
}

PlayListItem PlayList::itemAt(int row)
{
    if (mpModel->rowCount() < 0) {
        qWarning("Invalid rowCount");
        return PlayListItem();
    }
    return mpModel->data(mpModel->index(row), Qt::DisplayRole).value<PlayListItem>();
}

PlayListItem PlayList::itemAt(const QString &url)
{
    if (mpModel->rowCount() < 0)
    {
        qWarning("Invalid rowCount");
        return PlayListItem();
    }
    for (int i = mpModel->rowCount() - 1; i >= 0; --i)
    {
        PlayListItem item = mpModel->data(mpModel->index(i), Qt::DisplayRole).value<PlayListItem>();
        if (item.url() == url)
        {
            return mpModel->data(mpModel->index(i), Qt::DisplayRole).value<PlayListItem>();
        }
    }
    return PlayListItem();
}

void PlayList::insertItemAt(const PlayListItem &item, int row)
{
    if (mMaxRows > 0 && mpModel->rowCount() >= mMaxRows) {
        // +1 because new row is to be inserted
        mpModel->removeRows(mMaxRows, mpModel->rowCount() - mMaxRows + 1);
    }
    int i = mpModel->items().indexOf(item, row+1);
    if (i > 0) {
        mpModel->removeRow(i);
    }
    if (!mpModel->insertRow(row))
        return;
    if (row > 0) {
        i = mpModel->items().lastIndexOf(item, row - 1);
        if (i >= 0)
            mpModel->removeRow(i);
    }
    setItemAt(item, row);
}

void PlayList::setItemAt(const PlayListItem &item, int row)
{
    mpModel->setData(mpModel->index(row), QVariant::fromValue(item), Qt::DisplayRole);
}

void PlayList::insert(const QString &url, int row)
{
    PlayListItem item;
    item.setUrl(url);
    item.setDuration(0);
    item.setLastTime(0);
    QString title = url;
    if (!url.contains(QLatin1String("://")) || url.startsWith(QLatin1String("file://"))) {
        title = QFileInfo(url).fileName();
    }
    item.setTitle(title);
    insertItemAt(item, row);
}

void PlayList::remove(const QString &url)
{
    for (int i = mpModel->rowCount() - 1; i >= 0; --i) {
        PlayListItem item = mpModel->data(mpModel->index(i), Qt::DisplayRole).value<PlayListItem>();
        if (item.url() == url) {
            mpModel->removeRow(i);
        }
    }
}

void PlayList::setMaxRows(int r)
{
    mMaxRows = r;
}

int PlayList::maxRows() const
{
    return mMaxRows;
}

int PlayList::rowCount() const
{
    return mpModel->rowCount();
}

void PlayList::cleanAndDelete()
{
    clearItems();
    deleteSaveFile();
}

void PlayList::removeAll()
{
    clearItems();
}

void PlayList::removeSelectedItems()
{
    QItemSelectionModel *selection = mpListView->selectionModel();
    if (!selection->hasSelection())
        return;
    QModelIndexList s = selection->selectedIndexes();
    for (int i = s.size()-1; i >= 0; --i) {
        mpModel->removeRow(s.at(i).row());
    }
}

void PlayList::clearItems()
{
    mpModel->removeRows(0, mpModel->rowCount());
}

void PlayList::addItems()
{
    // TODO: add url;
    QStringList files = QFileDialog::getOpenFileNames(0, tr("Select files"));
    if (files.isEmpty())
        return;
    // TODO: check playlist file: m3u, pls... In another thread
    for (int i = 0; i < files.size(); ++i) {
        QString file = files.at(i);
        if (!QFileInfo(file).isFile())
            continue;
        insert(file, i);
    }
}

void PlayList::onAboutToPlay(const QModelIndex &index)
{
    emit aboutToPlay(index.data(Qt::DisplayRole).value<PlayListItem>().url());
    save();
}


