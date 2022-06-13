#!/usr/bin/python3

import sys

from tabulate import tabulate

def char_hex(x):
    # if we want '0x' prepended: format(255, '#04x')
    return format(x, '02x')

if len(sys.argv) == 2:
    # cmd line parameter
    cmd_str = sys.argv[1]
    cmd_arr = bytes.fromhex(cmd_str)
    if (len(cmd_arr) < 256):
        print("Tamanho do comando < 256!")
        sys.exit(1)
else:
    # read from stdin
    # 'sys.stdin.read()' can't read binary data properly
    cmd_arr = sys.stdin.buffer.raw.read(256)

# print(cmd_arr)
# sys.exit(1)
#for x in cmd_arr:
#    print(x)

table = []


def arr2float(arr):
    barr = bytearray(arr)
    import struct
    return struct.unpack("<f", arr)[0]


def print2table_14(table, cmd):
    table.extend([
        [
            "Timestamp das grandezas", 
            "{:02x}:{:02x}:{:02x} {:02x}/{:02x}/{:02x}".format(
                cmd[5], 
                cmd[6],
                cmd[7],
                cmd[8],
                cmd[9],
                cmd[10]
            )
        ],
        [
            "Tensão de fase A",
            arr2float(cmd[11:15])
        ],
        [
            "Tensão de fase B",
            arr2float(cmd[15:19])
        ],
        [
            "Tensão de fase C",
            arr2float(cmd[19:23])
        ],
        [
            "Tensão de linha AB",
            arr2float(cmd[23:27])
        ],
        [
            "Tensão de linha BC",
            arr2float(cmd[27:31])
        ],
        [
            "Tensão de linha CA",
            arr2float(cmd[31:35])
        ],
        [
            "Corrente de fase A",
            arr2float(cmd[35:39])
        ],
        [
            "Corrente de fase B",
            arr2float(cmd[39:43])
        ],
        [
            "Corrente de fase C",
            arr2float(cmd[43:47])
        ],
        [
            "Corrente de neutro",
            arr2float(cmd[47:51])
        ],
        [
            "Potência ativa fase A",
            arr2float(cmd[51:55])
        ],
        [
            "Potência ativa fase B",
            arr2float(cmd[55:59])
        ],
        [
            "Potência ativa fase C",
            arr2float(cmd[59:63])
        ],
        [
            "Potência ativa trifásica",
            arr2float(cmd[63:67])
        ],
        [
            "Potência reativa fase A",
            arr2float(cmd[67:71])
        ],
        [
            "Potência reativa fase B",
            arr2float(cmd[71:75])
        ],
        [
            "Potência reativa fase C",
            arr2float(cmd[75:79])
        ],
        [
            "Potência reativa trifásica",
            arr2float(cmd[79:83])
        ],
        [
            "Potência aparente quadrática fase A",
            arr2float(cmd[83:87])
        ],
        [
            "Potência aparente quadrática fase B",
            arr2float(cmd[87:91])
        ],
        [
            "Potência aparente quadrática fase C",
            arr2float(cmd[91:95])
        ],
        [
            "Potência aparente quadrática trifásica",
            arr2float(cmd[95:99])
        ],
        [
            "Potência aparente vetorial fase A",
            arr2float(cmd[99:103])
        ],
        [
            "Potência aparente vetorial fase B",
            arr2float(cmd[103:107])
        ],
        [
            "Potência aparente vetorial fase C",
            arr2float(cmd[107:111])
        ],
        [
            "Potência aparente vetorial trifásica",
            arr2float(cmd[111:115])
        ],
        [
            "Potência distorsiva fase A",
            arr2float(cmd[115:119])
        ],
        [
            "Potência distorsiva fase B",
            arr2float(cmd[119:123])
        ],
        [
            "Potência distorsiva fase C",
            arr2float(cmd[123:127])
        ],
        [
            "Potência distorsiva trifásica",
            arr2float(cmd[127:131])
        ],
        [
            "Cosseno Fi fase A",
            arr2float(cmd[131:135])
        ],
        [
            "Cosseno Fi fase B",
            arr2float(cmd[135:139])
        ],
        [
            "Cosseno Fi fase C",
            arr2float(cmd[139:143])
        ],
        [
            "Cosseno Fi trifásico",
            arr2float(cmd[143:147])
        ]
    ])

def print2table_20_21_22_51(table, cmd):
    table.extend([
        [
            "Timestamp atual", 
            "{:02x}:{:02x}:{:02x} {:02x}/{:02x}/{:02x}".format(
                cmd[5], 
                cmd[6],
                cmd[7],
                cmd[8],
                cmd[9],
                cmd[10]
            )
        ],
        [
            "Fim do último intervalo de memória de massa",
            "{:02x}:{:02x}:{:02x} {:02x}/{:02x}/{:02x}".format(
                cmd[12], 
                cmd[13],
                cmd[14],
                cmd[15],
                cmd[16],
                cmd[17],
            )
        ],
        [
            "Última reposição de demanda",
            "{:02x}:{:02x}:{:02x} {:02x}/{:02x}/{:02x}".format(
                cmd[18],
                cmd[19],
                cmd[20],
                cmd[21],
                cmd[22],
                cmd[23],
            )
        ],
        [
            "Penúltima reposição de demanda",
            "{:02x}:{:02x}:{:02x} {:02x}/{:02x}/{:02x}".format(
                cmd[24],
                cmd[25],
                cmd[26],
                cmd[27],
                cmd[28],
                cmd[29],
            )
        ],
        [
            "Intervalo de demanda atual em minutos",
            "{:02x}".format(cmd[81])
        ],
        [
            "Intervalo de demanda anterior em minutos",
            "{:02x}".format(cmd[82])
        ],
        [
            "Número de operações de reposição de demanda",
            "{:02x}".format(cmd[80])
        ],
        [
            "Número de palavras de 12 bits da leitura atual",
            "{:02x}{:02x}{:02x}".format(cmd[74], cmd[75], cmd[76])
        ],
        [
            "Número de palavras de 12 bits da última reposição de demanda",
            "{:02x}{:02x}{:02x}".format(cmd[77], cmd[78], cmd[79])
        ],
        [
            "Nº de grupos de canais disponíveis",
            "{:02x}".format(cmd[246])
        ],
        [
            "Código de grandeza do 1º, 2º e 3º canal",
            "{:02x}, {:02x}, {:02x}".format(cmd[195], cmd[196], cmd[197])
        ],
        [
            "Dia da reposição de demanda automática",
            "{:02x}".format(cmd[156])
        ],
        [
            "Nº de minutos, segundos e centésimos de segundo do intervalo da memória de massa",
            "{:02x}, {:02x}, {:02x}".format(cmd[203], cmd[204], cmd[205])
        ]
    ])

code2print = {
    0x14: {
        'title': "Grandezas Instantâneas",
        'print2table': print2table_14
    },
    0x20: {
        'title': "Parâmetros com reposição de demanda",
        'print2table': print2table_20_21_22_51
    },
    0x21: {
        'title': "Parâmetros sem reposição de demanda atuais",
        'print2table': print2table_20_21_22_51
    },
    0x22: {
        'title': "Parâmetros sem reposição de demanda anteriores",
        'print2table': print2table_20_21_22_51
    },
    0x51: {
        'title': "Parâmetros sem reposição de demanda, com leitura de memória de massa",
        'print2table': print2table_20_21_22_51
    }
}

code = cmd_arr[0]
if code in code2print:
    print(code2print[code]['title'])
    code2print[code]['print2table'](table, cmd_arr)
    print(tabulate(table, headers=['Campo', 'Valor'], tablefmt='orgtbl'));
else:
    print("Comando não implementado")
    sys.exit(1)

sys.exit(0)