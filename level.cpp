#include "level.h"

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>

#include "parameter.h"

const QSize Level::SIZE = QSize(20, 15);

Level::Level(const QString &name, QSize size):
	name(name),
	isSaved(false)
{
	this->init(size);
}

Level::Level(const QString &name, const QString &path):
	name(name),
	path(path),
	isSaved(true)
{}

Level::~Level()
{}

void Level::init(QSize size)
{
	std::vector<ElementDesc> record;
	record.assign(size.width(), ElementDesc(QString(), QHash<QString, QString>()));
	this->elements.assign(size.height(), record);
}

void Level::load()
{
	QFile file(path + QDir::separator() + name);
	if (!file.exists())
	{
		qDebug() << "File with level " << name << " does not exist";
		return;
	}

	QDataStream stream(&file);
	stream.device()->open(QIODevice::ReadOnly);

	size_t width = 0;
	size_t height = 0;

	stream >> width >> height;

	this->init(QSize(width, height));

	for (size_t h = 0; h < height; ++h)
	{
		for (size_t w = 0; w < width; ++w)
		{
			QString name;
			stream >> name;

			this->elements[h][w] = ElementDesc(name, QHash<QString, QString>());

			emit elementLoaded(name, QPoint(w, h));
		}
	}

	qDebug() << "Level " + this->name + " loaded";

	stream.device()->close();
}

QSize Level::getSize() const
{
	size_t height = this->elements.size();
	size_t width = (height > 0) ? this->elements[0].size() : 0;

	return QSize(width, height);
}

const ElementDesc& Level::select(QPoint position)
{
	return this->elements[position.y()][position.x()];
}

bool Level::isNew() const
{
	return this->path.isNull();
}

bool Level::isChanged() const
{
	return !this->isSaved;
}

void Level::add(const Element &element, QPoint place)
{
	QHash<QString, QString> paramsValues;
	for (auto param : element.getParameters())
	{
		paramsValues[param->getName()] = param->getDefault();
	}

	this->elements[place.y()][place.x()] = ElementDesc(element.getName(), paramsValues);

	this->isSaved = false;
}

void Level::save(QString newName, QString newPath)
{
	this->name = newName;
	this->path = newPath;

	QFile file(this->path + QDir::separator() + this->name);
	QDataStream stream(&file);
	stream.device()->open(QIODevice::WriteOnly);

	QSize size = this->getSize();

	stream << size.width() << size.height();

	for (int h = 0; h < size.height(); ++h)
	{
		for (int w = 0; w < size.width(); ++w)
		{
			stream << this->elements[h][w].getName();
		}
	}

	this->isSaved = true;

	qDebug() << "Level " + this->name + " saved";

	stream.device()->close();
}
