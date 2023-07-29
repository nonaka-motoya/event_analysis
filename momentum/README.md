# momentum

`calc_momentum.cpp`: vertex fileを読み込んで運動量を測定し、p_recを詰めます

`ratio_p_true_p_rec.cpp`: p_recの詰められたvertex fileを読み込み、割合を計算します

## Usage

### 1. linked_tracksのパスのリストを作成
linked_tracks.rootのパスを全てテキストファイルに書き出す必要があります。

例：[/input_files/LTList.txt.debug](https://github.com/nonaka-motoya/event_analysis/blob/master/momentum/input_files/LTList.txt.debug)

直接テキストファイルを作成するか、[create_input_path.sh](https://github.com/nonaka-motoya/event_analysis/blob/master/momentum/create_input_path.sh)を使ってください。

使用例：
```shell
source create_input_path.sh /mnt/004_Disk2/FASERnu_MC/20230212_nuall_00010-00019_p500/
source create_input_path.sh /mnt/004_Disk2/FASERnu_MC/20230212_nuall_00020-00029_p500/
source create_input_path.sh /mnt/004_Disk2/FASERnu_MC/20230212_nuall_00030-00039_p500/
```
引数には`evt_*`フォルダがあるディレクトリを与えてください。出てくるテキストファイル名を変えたい場合は[create_input_path.sh](https://github.com/nonaka-motoya/event_analysis/blob/master/momentum/create_input_path.sh)のoutput_fileを変えてください。

### 2. p_recをvertex fileに詰める
[calc_mom.sh](https://github.com/nonaka-motoya/event_analysis/blob/master/momentum/calc_mom.sh)を編集してください。

例：
```
mkdir ./output
./calc_momentum -V ./input_files/vtx_info_nuall_00010-00039_p500_numucc_v20230706.txt -I ./input_files/LTList.txt.debug -O ./output/vtx_test.txt
```
説明:
* -V: vertex fileのパス
* -I: [Usage 1](https://github.com/nonaka-motoya/event_analysis/tree/master/momentum#1-linked_tracks%E3%81%AE%E3%83%91%E3%82%B9%E3%81%AE%E3%83%AA%E3%82%B9%E3%83%88%E3%82%92%E4%BD%9C%E6%88%90)で作成したlinked_tracks.rootのパスのリストのテキストファイルのパス
* -O: p_recの詰められたvertex fileの出力場所

編集後実行してください。
```
source calc_mom.sh
```

### 3. 生成したvertex fileを元に割合を計算
[calc_ratio.sh](https://github.com/nonaka-motoya/event_analysis/blob/master/momentum/calc_ratio.sh)を編集してください。

例：
```shell
./ratio_ptrue_prec -V ./output/vtx_info_nuall_00010-00039_p500_numucc_v20230706_measured_mometum_50plates.txt -O ./output/test_p_true_vs_p_rec.txt
```

説明：
* -V p_recの詰められたvertex fileのパス
* -O 出力ファイルのパス



編集後実行してください。
```shell
source calc_ratio.sh
```


実行後、以下のようなテキストファイルが出力されます。

```text
Event ID: 21131
PGD: -211	Npl: 16	P_true: 20.62	P_rec: 28.645
PGD: -321	Npl: 19	P_true: 4.47	P_rec: 5.78308
PGD: 211	Npl: 29	P_true: 16.57	P_rec: 10.93
PGD: 211	Npl: 48	P_true: 4.46	P_rec: 4.45542
PGD: 211	Npl: 20	P_true: 8.59	P_rec: 12.6
PGD: 211	Npl: 111	P_true: 2.42	P_rec: 2.00319
PGD: 13	Npl: 111	P_true: 846.49	P_rec: 664.329
======================================================================
Event ID: 21155
PGD: 13	Npl: 110	P_true: 104.92	P_rec: 90.2551
PGD: 211	Npl: 110	P_true: 15.14	P_rec: 13.2088
PGD: 211	Npl: 18	P_true: 13.96	P_rec: 16.7216
PGD: 11	Npl: 13	P_true: 4.61	P_rec: 1.96098
PGD: -321	Npl: 110	P_true: 71.1	P_rec: 67.4067
PGD: 211	Npl: 110	P_true: 3.09	P_rec: 3.26223
======================================================================

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Number of events: 1176
少なくとも1本p_true>200GeVのトラックがいるevent数: 810
すべてのトラックがp_true<200GeVのevent数: 366


| | P_rec > 200 | P_rec < 200 |
| --- | --- | --- |
| P_true > 200 | 726/810=0.896296 | 84/810=0.103704 |
| P_true < 200 | 90/366=0.245902 | 276/366=0.754098 |


Only muon tracks.
| | P_rec > 200 | P_rec < 200 |
| --- | --- | --- |
| P_true > 200 | 658/706=0.932011 | 48/706=0.0679887 |
| P_true < 200 | 44/470=0.093617 | 426/470=0.906383 |

```
