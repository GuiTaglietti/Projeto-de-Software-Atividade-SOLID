/**
 * @author Guilherme M. Taglietti
 * @brief  Banco simples com operações de depósito, saque, transferência e extrato.
 * @date   2023-10-01
 * @note   Atividade - Projeto de Software (SOLID APLICADO).
 */
#include <bits/stdc++.h>
using namespace std;

struct ValorInvalido : runtime_error { using runtime_error::runtime_error; };
struct SaldoInsuficiente : runtime_error { using runtime_error::runtime_error; };
struct EntidadeNaoEncontrada : runtime_error { using runtime_error::runtime_error; };

struct Cliente {
    int id;
    string nome;
    string cpf;
};

struct Transacao {
    chrono::system_clock::time_point momento;
    string tipo;
    long long centavos;
    string descricao;
    string origem;
    string destino;
};

static string format_money(long long centavos) {
    bool neg = centavos < 0;
    long long v = llabs(centavos);
    long long reais = v / 100;
    int c = v % 100;
    stringstream ss;
    if (neg) ss << "-";
    ss << reais << "." << setw(2) << setfill('0') << c;
    return ss.str();
}

struct IClock {
    virtual ~IClock() = default;
    virtual chrono::system_clock::time_point now() const = 0;
};

struct SystemClock final : IClock {
    chrono::system_clock::time_point now() const override { return chrono::system_clock::now(); }
};

struct IConta {
    virtual ~IConta() = default;
    virtual const string& numero() const = 0;
    virtual shared_ptr<Cliente> cliente() const = 0;
    virtual long long saldo_centavos() const = 0;
    virtual const vector<Transacao>& historico() const = 0;
    virtual void depositar(long long centavos, const string& descricao) = 0;
    virtual void sacar(long long centavos, const string& descricao) = 0;
    virtual void registrar(const Transacao& t) = 0;
};

class Conta final : public IConta {
    string numero_;
    shared_ptr<Cliente> cliente_;
    long long centavos_ = 0;
    vector<Transacao> historico_;
    const IClock& clock_;
public:
    Conta(string numero, shared_ptr<Cliente> cliente, const IClock& clock)
        : numero_(move(numero)), cliente_(move(cliente)), clock_(clock) {}
    const string& numero() const override { return numero_; }
    shared_ptr<Cliente> cliente() const override { return cliente_; }
    long long saldo_centavos() const override { return centavos_; }
    const vector<Transacao>& historico() const override { return historico_; }
    void depositar(long long centavos, const string& descricao) override {
        if (centavos <= 0) throw ValorInvalido("valor deve ser positivo");
        centavos_ += centavos;
        historico_.push_back({clock_.now(),"DEPOSITO",centavos,descricao,"",numero_});
    }
    void sacar(long long centavos, const string& descricao) override {
        if (centavos <= 0) throw ValorInvalido("valor deve ser positivo");
        if (centavos_ < centavos) throw SaldoInsuficiente("saldo insuficiente");
        centavos_ -= centavos;
        historico_.push_back({clock_.now(),"SAQUE",centavos,descricao,numero_,""});
    }
    void registrar(const Transacao& t) override { historico_.push_back(t); }
};

struct IClientesRepo {
    virtual ~IClientesRepo() = default;
    virtual shared_ptr<Cliente> getByCpf(const string& cpf) = 0;
    virtual shared_ptr<Cliente> add(const string& nome, const string& cpf) = 0;
    virtual vector<shared_ptr<Cliente>> all() = 0;
};

struct IContasRepo {
    virtual ~IContasRepo() = default;
    virtual IConta* add(string numero, shared_ptr<Cliente> cli) = 0;
    virtual IConta* find(const string& numero) = 0;
    virtual vector<IConta*> byCliente(const string& cpf) = 0;
    virtual vector<IConta*> all() = 0;
    virtual string nextNumero() = 0;
};

class ClientesMem final : public IClientesRepo {
    unordered_map<string, shared_ptr<Cliente>> map_;
    int seq_ = 1;
public:
    shared_ptr<Cliente> getByCpf(const string& cpf) override {
        auto it = map_.find(cpf);
        if (it == map_.end()) return nullptr;
        return it->second;
    }
    shared_ptr<Cliente> add(const string& nome, const string& cpf) override {
        auto it = map_.find(cpf);
        if (it != map_.end()) return it->second;
        auto c = make_shared<Cliente>(Cliente{seq_++, nome, cpf});
        map_[cpf] = c;
        return c;
    }
    vector<shared_ptr<Cliente>> all() override {
        vector<shared_ptr<Cliente>> v;
        v.reserve(map_.size());
        for (auto& kv : map_) v.push_back(kv.second);
        return v;
    }
};

class ContasMem final : public IContasRepo {
    unordered_map<string, unique_ptr<IConta>> map_;
    int seq_ = 1001;
    const IClock& clock_;
public:
    ContasMem(const IClock& clock) : clock_(clock) {}
    IConta* add(string numero, shared_ptr<Cliente> cli) override {
        map_[numero] = make_unique<Conta>(move(numero), move(cli), clock_);
        return map_.at(map_.rbegin()->first).get();
    }
    IConta* find(const string& numero) override {
        auto it = map_.find(numero);
        if (it == map_.end()) return nullptr;
        return it->second.get();
    }
    vector<IConta*> byCliente(const string& cpf) override {
        vector<IConta*> v;
        for (auto& kv : map_) if (kv.second->cliente()->cpf == cpf) v.push_back(kv.second.get());
        return v;
    }
    vector<IConta*> all() override {
        vector<IConta*> v;
        v.reserve(map_.size());
        for (auto& kv : map_) v.push_back(kv.second.get());
        return v;
    }
    string nextNumero() override {
        return to_string(seq_++);
    }
};

struct IMoeda {
    virtual ~IMoeda() = default;
    virtual long long paraCentavos(double valor) const = 0;
};

class MoedaBRL final : public IMoeda {
public:
    long long paraCentavos(double valor) const override {
        long long c = llround(valor * 100.0);
        if (c == 0) throw ValorInvalido("valor inválido");
        return c;
    }
};

class BancoService {
    IClientesRepo& clientes_;
    IContasRepo& contas_;
    const IMoeda& moeda_;
    const IClock& clock_;
public:
    BancoService(IClientesRepo& cr, IContasRepo& orc, const IMoeda& m, const IClock& clk)
        : clientes_(cr), contas_(orc), moeda_(m), clock_(clk) {}
    shared_ptr<Cliente> criar_cliente(const string& nome, const string& cpf) {
        auto c = clientes_.getByCpf(cpf);
        if (c) return c;
        return clientes_.add(nome, cpf);
    }
    IConta* abrir_conta(const string& cpf) {
        auto c = clientes_.getByCpf(cpf);
        if (!c) throw EntidadeNaoEncontrada("cliente não encontrado");
        auto numero = contas_.nextNumero();
        return contas_.add(numero, c);
    }
    IConta* buscar_conta(const string& numero) {
        auto* c = contas_.find(numero);
        if (!c) throw EntidadeNaoEncontrada("conta não encontrada");
        return c;
    }
    void depositar(const string& numero, double valor) {
        buscar_conta(numero)->depositar(moeda_.paraCentavos(valor), "depósito");
    }
    void sacar(const string& numero, double valor) {
        buscar_conta(numero)->sacar(moeda_.paraCentavos(valor), "saque");
    }
    void transferir(const string& origem, const string& destino, double valor) {
        if (origem == destino) throw ValorInvalido("contas iguais");
        long long c = moeda_.paraCentavos(valor);
        auto* co = buscar_conta(origem);
        auto* cd = buscar_conta(destino);
        co->sacar(c, string("transferência para ") + destino);
        cd->depositar(c, string("transferência de ") + origem);
        Transacao t{clock_.now(),"TRANSFERENCIA",c,"transferência",origem,destino};
        co->registrar(t);
        cd->registrar(t);
    }
    vector<pair<shared_ptr<Cliente>, vector<IConta*>>> listar_clientes_e_contas() {
        vector<pair<shared_ptr<Cliente>, vector<IConta*>>> out;
        for (auto& cli : clientes_.all()) {
            out.push_back({cli, contas_.byCliente(cli->cpf)});
        }
        return out;
    }
    vector<Transacao> extrato(const string& numero) {
        return buscar_conta(numero)->historico();
    }
    static BancoService criar_dados_mock(BancoService svc) {
        auto a = svc.criar_cliente("Ana","11111111111");
        auto c1 = svc.abrir_conta(a->cpf)->numero();
        auto br = svc.criar_cliente("Bruno","22222222222");
        auto c2 = svc.abrir_conta(br->cpf)->numero();
        auto ca = svc.criar_cliente("Carla","33333333333");
        auto c3 = svc.abrir_conta(ca->cpf)->numero();
        svc.depositar(c1, 1500.00);
        svc.depositar(c2, 800.00);
        svc.depositar(c3, 2500.00);
        svc.transferir(c3, c1, 300.00);
        svc.sacar(c2, 100.00);
        return svc;
    }
};

int main() {
    SystemClock clock;
    MoedaBRL moeda;
    ClientesMem clientes;
    ContasMem contas(clock);
    BancoService svc(clientes, contas, moeda, clock);
    svc = BancoService::criar_dados_mock(move(svc));
    auto lista = svc.listar_clientes_e_contas();
    for (auto& par : lista) {
        auto cli = par.first;
        cout << cli->id << " " << cli->nome << " " << cli->cpf << " -> ";
        vector<string> contasv;
        for (auto* c : par.second) contasv.push_back(c->numero() + ":" + format_money(c->saldo_centavos()));
        cout << "[" << (contasv.empty() ? "" : contasv[0]);
        for (size_t i=1;i<contasv.size();++i) cout << ", " << contasv[i];
        cout << "]\n";
    }
    if (!lista.empty() && !lista.front().second.empty()) {
        auto num = lista.front().second.front()->numero();
        auto ext = svc.extrato(num);
        for (auto& t : ext) {
            cout << num << " " << t.tipo << " " << format_money(t.centavos) << " " << t.descricao << "\n";
        }
    }
    return 0;
}
