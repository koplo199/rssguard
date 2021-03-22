// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formcategorydetails.h"

#include "core/feedsmodel.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/baselineedit.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/rootitem.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>

FormCategoryDetails::FormCategoryDetails(ServiceRoot* service_root, RootItem* parent_to_select, QWidget* parent)
  : QDialog(parent), m_category(nullptr), m_serviceRoot(service_root), m_parentToSelect(parent_to_select) {
  initialize();
  createConnections();

  // Initialize text boxes.
  onTitleChanged(QString());
  onDescriptionChanged(QString());
}

FormCategoryDetails::~FormCategoryDetails() {
  qDebugNN << LOGSEC_GUI << "Destroying FormCategoryDetails instance.";
}

void FormCategoryDetails::createConnections() {
  // General connections.
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormCategoryDetails::apply);
  connect(m_ui->m_txtTitle->lineEdit(), &BaseLineEdit::textChanged,
          this, &FormCategoryDetails::onTitleChanged);
  connect(m_ui->m_txtDescription->lineEdit(), &BaseLineEdit::textChanged,
          this, &FormCategoryDetails::onDescriptionChanged);

  // Icon connections.
  connect(m_actionLoadIconFromFile, &QAction::triggered, this, &FormCategoryDetails::onLoadIconFromFile);
  connect(m_actionUseDefaultIcon, &QAction::triggered, this, &FormCategoryDetails::onUseDefaultIcon);
}

void FormCategoryDetails::loadCategoryData() {
  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot, m_category);

  if (m_creatingNew) {
    setWindowTitle(tr("Add new category"));

    // Make sure that "default" icon is used as the default option for new
    // categories.
    m_actionUseDefaultIcon->trigger();

    // Load parent from suggested item.
    if (m_parentToSelect != nullptr) {
      if (m_parentToSelect->kind() == RootItem::Kind::Category) {
        m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) m_parentToSelect)));
      }
      else if (m_parentToSelect->kind() == RootItem::Kind::Feed) {
        int target_item = m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) m_parentToSelect->parent()));

        if (target_item >= 0) {
          m_ui->m_cmbParentCategory->setCurrentIndex(target_item);
        }
      }
    }
  }
  else {
    setWindowTitle(tr("Edit '%1'").arg(m_category->title()));
    m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) m_category->parent())));
  }

  m_ui->m_txtTitle->lineEdit()->setText(m_category->title());
  m_ui->m_txtDescription->lineEdit()->setText(m_category->description());
  m_ui->m_btnIcon->setIcon(m_category->icon());
}

void FormCategoryDetails::apply() {
  RootItem* parent = static_cast<RootItem*>(m_ui->m_cmbParentCategory->itemData(m_ui->m_cmbParentCategory->currentIndex()).value<void*>());

  m_category->setTitle(m_ui->m_txtTitle->lineEdit()->text());
  m_category->setCreationDate(QDateTime::currentDateTime());
  m_category->setDescription(m_ui->m_txtDescription->lineEdit()->text());
  m_category->setIcon(m_ui->m_btnIcon->icon());

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::createOverwriteCategory(database, m_category, m_serviceRoot->accountId(), parent->id());
  }
  catch (const ApplicationException& ex) {
    qFatal("Cannot save category: '%s'.", qPrintable(ex.message()));
  }

  m_serviceRoot->requestItemReassignment(m_category, parent);
  m_serviceRoot->itemChanged({ m_category });
  accept();
}

void FormCategoryDetails::onTitleChanged(const QString& new_title) {
  if (!new_title.simplified().isEmpty()) {
    m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(true);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::StatusType::Ok, tr("Category name is ok."));
  }
  else {
    m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::StatusType::Error, tr("Category name is too short."));
  }
}

void FormCategoryDetails::onDescriptionChanged(const QString& new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Warning, tr("Description is empty."));
  }
  else {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Ok, tr("The description is ok."));
  }
}

void FormCategoryDetails::onLoadIconFromFile() {
  QFileDialog dialog(this, tr("Select icon file for the category"),
                     qApp->homeFolder(), tr("Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)"));

  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setWindowIcon(qApp->icons()->fromTheme(QSL("image-x-generic")));
  dialog.setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly);
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setLabelText(QFileDialog::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));

  //: Label to describe the folder for icon file selection dialog.
  dialog.setLabelText(QFileDialog::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::Accepted) {
    m_ui->m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void FormCategoryDetails::onUseDefaultIcon() {
  m_ui->m_btnIcon->setIcon(QIcon());
}

void FormCategoryDetails::initialize() {
  m_ui.reset(new Ui::FormCategoryDetails());
  m_ui->setupUi(this);

  // Set text boxes.
  m_ui->m_txtTitle->lineEdit()->setPlaceholderText(tr("Category title"));
  m_ui->m_txtTitle->lineEdit()->setToolTip(tr("Set title for your category."));
  m_ui->m_txtDescription->lineEdit()->setPlaceholderText(tr("Category description"));
  m_ui->m_txtDescription->lineEdit()->setToolTip(tr("Set description for your category."));

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("folder")));

  // Setup button box.
  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile = new QAction(qApp->icons()->fromTheme(QSL("image-x-generic")),
                                         tr("Load icon from file..."),
                                         this);
  m_actionUseDefaultIcon = new QAction(qApp->icons()->fromTheme(QSL("folder")),
                                       tr("Use default icon from icon theme"),
                                       this);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  m_ui->m_btnIcon->setMenu(m_iconMenu);

  // Setup tab order.
  setTabOrder(m_ui->m_cmbParentCategory, m_ui->m_txtTitle->lineEdit());
  setTabOrder(m_ui->m_txtTitle->lineEdit(), m_ui->m_txtDescription->lineEdit());
  setTabOrder(m_ui->m_txtDescription->lineEdit(), m_ui->m_btnIcon);
  setTabOrder(m_ui->m_btnIcon, m_ui->m_buttonBox);
  m_ui->m_txtTitle->lineEdit()->setFocus(Qt::TabFocusReason);
}

void FormCategoryDetails::loadCategories(const QList<Category*>& categories,
                                         RootItem* root_item,
                                         Category* input_category) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));

  for (Category* category : categories) {
    if (input_category != nullptr && (category == input_category || category->isChildOf(input_category))) {
      // This category cannot be selected as the new
      // parent for currently edited category, so
      // don't add it.
      continue;
    }

    m_ui->m_cmbParentCategory->addItem(category->data(FDS_MODEL_TITLE_INDEX, Qt::DecorationRole).value<QIcon>(),
                                       category->title(),
                                       QVariant::fromValue((void*) category));
  }
}
