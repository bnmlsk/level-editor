#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QUrl>

const QString MainWindow::LEVEL_FILE_FILTER = "Levels (*.level)";

void MainWindow::extarctNameAndPath(QString source, QString &name, QString &path)
{
	source = QDir::toNativeSeparators(source);
	int lastSeparatorIndex = source.lastIndexOf(QDir::separator());

	path = source.mid(0, lastSeparatorIndex);
	name = source.mid(lastSeparatorIndex + 1, source.size() - lastSeparatorIndex);
}

MainWindow::MainWindow(QWidget *parent):
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	level(nullptr)
{
	this->ui->setupUi(this);

	this->drawArea = new DrawArea(this);

	this->ui->layoutDrawArea->addWidget(this->drawArea);

	this->drawArea->resize(Level::SIZE * DrawArea::PROPORTION);

	this->updateElementsList();

	this->setMenuActionsState(LevelUnloaded);
}

MainWindow::~MainWindow()
{
	delete this->drawArea;

	delete ui;
}

void MainWindow::selectElement(QPoint position)
{
	auto &elementDesc = this->level->select(position);
}

void MainWindow::placeElementOnLevel(const QString &name, QPoint position)
{
	this->level->add(name, position);

	this->setMenuActionsState(LevelChanged);
}

void MainWindow::placeLoadedElement(const QString &name, QPoint position)
{
	auto element_iter = this->elements.constFind(name);
	if (element_iter == this->elements.end())
	{
		return;
	}

	qDebug() << "Element with name " << name;

	this->drawArea->setCurrentElement(element_iter.value());
	this->drawArea->setCurrentPosition(position);

	this->drawArea->repaint();
}

void MainWindow::on_actionNewLevel_triggered()
{
	if (!this->closeLevel())
		return;

	QString name = QInputDialog::getText(this, "Новый уровень", "Введите имя нового уровня");
	if (name.isEmpty())
		return;

	this->level = new Level(this->elements, name, Level::SIZE);

	this->setMenuActionsState(LevelChanged);

	this->bindSlots();
}

void MainWindow::on_actionSaveLevel_triggered()
{
	QString saveName = QFileDialog::getSaveFileName(this, tr("Сохранить"), QString(), LEVEL_FILE_FILTER);
	if (saveName.isEmpty())
		return;

	QString name;
	QString path;

	extarctNameAndPath(saveName, name, path);

	this->level->save(name, path);

	this->setMenuActionsState(LevelLoaded);
}

void MainWindow::on_actionLoadLevel_triggered()
{
	if (!this->closeLevel())
		return;

	QString url = QFileDialog::getOpenFileName(this, tr("Открыть"), QString(), LEVEL_FILE_FILTER);
	if (url.isEmpty())
		return;

	QString name;
	QString path;

	extarctNameAndPath(url, name, path);

	this->level = new Level(this->elements, name, path);

	this->bindSlots();

	this->level->load();

	this->setMenuActionsState(LevelLoaded);
}

void MainWindow::on_listElements_itemClicked(QListWidgetItem *item)
{
	this->setActionChoosed(ActionPaint);

	auto elementIter = this->elements.find(item->text());
	if (elementIter == this->elements.end())
	{
		qDebug() << "No element with name " << item->text();
		return;
	}

	this->drawArea->setCurrentElement(elementIter.value());
}

void MainWindow::on_buttonEraser_clicked()
{
	this->setActionChoosed(ActionErase);

	this->drawArea->setEraser();
}

void MainWindow::on_buttonSelect_clicked()
{
	this->setActionChoosed(ActionSelect);

	this->drawArea->startSelecting();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (this->closeLevel())
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

bool MainWindow::closeLevel()
{
	if (this->level == nullptr)
		return true;

	bool toClose = true;
	if (this->level->isChanged())
	{
		QMessageBox::StandardButton reply = QMessageBox::question(this, "Изменения", "Имеются не сохраненные изменения. Сохранить?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		switch (reply)
		{
		case QMessageBox::Yes:
			this->on_actionSaveLevel_triggered();
			toClose = true;
			break;
		case QMessageBox::No:
			toClose = true;
			break;
		case QMessageBox::Cancel:
			toClose = false;
			break;
		default:
			return false;
		}
	}

	if (toClose)
		delete this->level;

	return toClose;
}

void MainWindow::bindSlots()
{
	QObject::connect(this->drawArea, SIGNAL(elementPlaced(QString, QPoint)), this, SLOT(placeElementOnLevel(QString, QPoint)));
	QObject::connect(this->drawArea, SIGNAL(elementSelected(QPoint)), this, SLOT(selectElement(QPoint)));

	QObject::connect(this->level, SIGNAL(elementLoaded(QString, QPoint)), this, SLOT(placeLoadedElement(QString, QPoint)));
}

void MainWindow::loadElement(const QString &elementName)
{
	QString pathToLevel = this->config.getElementsDictory() + QDir::separator() + elementName;
	QPixmap pixmap(pathToLevel + QDir::separator() + this->config.getElementPictureName());
	if (pixmap.isNull())
	{
		qDebug() << "Pixmap for element " << elementName << " not found.";
		return;
	}

	this->elements.insert(elementName, Element(elementName, pixmap));

	this->ui->listElements->addItem(elementName);

	this->setMenuActionsState(LevelLoaded);
}

void MainWindow::setMenuActionsState(MenuState state)
{
	this->drawArea->setEnabled(LevelUnloaded != state);
	this->ui->actionSaveLevel->setEnabled(LevelChanged == state);
}

void MainWindow::setActionChoosed(Action action)
{
	this->ui->listElements->setItemSelected(this->ui->listElements->currentItem(), ActionPaint == action);
	this->ui->buttonSelect->setDown(ActionSelect == action);
	this->ui->buttonEraser->setDown(ActionErase == action);
}

void MainWindow::updateElementsList()
{
	QDir directory(this->config.getElementsDictory());
	auto elementsDirectory = directory.entryList(QStringList("[A-Za-z]*"));
	for (auto elementName : elementsDirectory)
	{
		this->loadElement(elementName);
	}
}
