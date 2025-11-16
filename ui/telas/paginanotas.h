#pragma once
#include <QWidget>
#include <QString>

class QTableView;
class QStandardItemModel;
class QPushButton;

namespace Ui { class PaginaNotas; }

class PaginaNotas : public QWidget {
    Q_OBJECT
public:
    explicit PaginaNotas(QWidget* parent = nullptr);
    ~PaginaNotas();

private slots:
    void onNovo();
    void onEditar();
    void onRemover();
    void onRecarregar();
private:
    Ui::PaginaNotas*    ui;
    QTableView*         m_table{};
    QStandardItemModel* m_model{};
    QPushButton *m_btnNovo{}, *m_btnEditar{}, *m_btnRemover{}, *m_btnRecarregar{};
    int m_nextId{1};

    const QString m_arquivo = "notas.txt";
    bool salvarNoArquivo() const;
    bool carregarDoArquivo();
    void recomputarNextId();

    int  selectedRow() const;
    void addNota(const QString& projeto, const QString& avaliador, double media);
};
