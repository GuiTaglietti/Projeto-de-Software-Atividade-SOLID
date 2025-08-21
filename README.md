# Projeto: Aplicação dos Princípios SOLID

## Estrutura:

```
atividade-solid/
├── original/OriginalClasses.cpp
├── solid/RefactoredClasses.cpp
└── readme.md
```

## Objetivo
Refatorar o código da atividade da semana passada (um banco simples) aplicando SOLID (SRP, OCP, LSP, ISP, DIP), mantendo um executável funcional e equivalência comportamental.

## Conta

### a) Classe Original
```cpp
class Conta {
    string numero_;
    shared_ptr<Cliente> cliente_;
    long long centavos_ = 0;
    vector<Transacao> historico_;
public:
    Conta(string numero, shared_ptr<Cliente> cliente);
    void depositar(long long centavos, const string& descricao = "");
    void sacar(long long centavos, const string& descricao = "");
    void registrar(const Transacao& t);
};
```

### b) Princípios SOLID Aplicados

**Princípios:** SRP, DIP, ISP, LSP

**Explicação:** Isolamos acesso ao tempo via `IClock` e definimos `IConta` para separar contrato de implementação (ISP/LSP). A dependência para captura de tempo é invertida (DIP), permitindo testes determinísticos. A classe mantém responsabilidade única de saldo e histórico (SRP).

### c) Classe Refatorada
```cpp
struct IConta {
    virtual ~IConta()=default;
    virtual const string& numero() const=0;
    virtual shared_ptr<Cliente> cliente() const=0;
    virtual long long saldo_centavos() const=0;
    virtual const vector<Transacao>& historico() const=0;
    virtual void depositar(long long,const string&)=0;
    virtual void sacar(long long,const string&)=0;
    virtual void registrar(const Transacao&)=0;
};

class Conta final : public IConta {
    string numero_;
    shared_ptr<Cliente> cliente_;
    long long centavos_=0;
    vector<Transacao> historico_;
    const IClock& clock_;
public:
    Conta(string numero, shared_ptr<Cliente> cliente, const IClock& clock);
    const string& numero() const override;
    shared_ptr<Cliente> cliente() const override;
    long long saldo_centavos() const override;
    const vector<Transacao>& historico() const override;
    void depositar(long long,const string&) override;
    void sacar(long long,const string&) override;
    void registrar(const Transacao&) override;
};
```

## Banco/BancoService

### a) Classe Original
```cpp
class Banco {
    unordered_map<string, shared_ptr<Cliente>> clientes_;
    unordered_map<string, unique_ptr<Conta>> contas_;
    int seq_cliente_=1, seq_conta_=1001;
    static long long to_centavos(double);
public:
    shared_ptr<Cliente> criar_cliente(const string&,const string&);
    Conta* abrir_conta(const string&);
    Conta* buscar_conta(const string&);
    void depositar(const string&, double);
    void sacar(const string&, double);
    void transferir(const string&, const string&, double);
    vector<pair<shared_ptr<Cliente>, vector<Conta*>>> listar_clientes_e_contas();
    vector<Transacao> extrato(const string&);
    static Banco criar_dados_mock();
};
```

### b) Princípios SOLID Aplicados

**Princípios:** SRP, OCP, DIP, ISP

**Explicação:** Separamos persistência em repositórios (`IClientesRepo`, `IContasRepo`) e moeda (`IMoeda`), invertendo dependências. `BancoService` coordena casos de uso dependendo de interfaces (DIP), permitindo trocar armazenamento e política de numeração de contas sem mexer na lógica (OCP). Responsabilidades ficam claras entre serviço e repositórios (SRP). Interfaces específicas por papel (ISP).

### c) Classe Refatorada
```cpp
struct IClientesRepo { virtual ~IClientesRepo()=default; virtual shared_ptr<Cliente> getByCpf(const string&)=0; virtual shared_ptr<Cliente> add(const string&,const string&)=0; virtual vector<shared_ptr<Cliente>> all()=0; };
struct IContasRepo { virtual ~IContasRepo()=default; virtual IConta* add(string,shared_ptr<Cliente>)=0; virtual IConta* find(const string&)=0; virtual vector<IConta*> byCliente(const string&)=0; virtual vector<IConta*> all()=0; virtual string nextNumero()=0; };
struct IMoeda { virtual ~IMoeda()=default; virtual long long paraCentavos(double) const=0; };

class BancoService {
    IClientesRepo& clientes_;
    IContasRepo& contas_;
    const IMoeda& moeda_;
    const IClock& clock_;
public:
    BancoService(IClientesRepo&, IContasRepo&, const IMoeda&, const IClock&);
    shared_ptr<Cliente> criar_cliente(const string&,const string&);
    IConta* abrir_conta(const string&);
    IConta* buscar_conta(const string&);
    void depositar(const string&, double);
    void sacar(const string&, double);
    void transferir(const string&, const string&, double);
    vector<pair<shared_ptr<Cliente>, vector<IConta*>>> listar_clientes_e_contas();
    vector<Transacao> extrato(const string&);
    static BancoService criar_dados_mock(BancoService);
};
```

## Moeda

### a) Versão Original

Função estática embutida em Banco realizando conversão para centavos.

### b) Princípio SOLID Aplicado

**Princípios:** SRP, DIP, OCP

**Explicação:** Extraímos a política de conversão monetária para `IMoeda`/`MoedaBRL`. O serviço depende de uma abstração, permitindo novas moedas sem alterar a lógica central (OCP).

### c) Classe Refatorada
```cpp
struct IMoeda { virtual ~IMoeda()=default; virtual long long paraCentavos(double) const=0; };
class MoedaBRL final : public IMoeda { public: long long paraCentavos(double v) const override; };
```

