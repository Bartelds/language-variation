# Quantifying Language Variation Acoustically with Few Resources
Code associated with the paper: Quantifying Language Variation Acoustically with Few Resources.

Accepted at NAACL 2022.

 > **Abstract**: Deep acoustic models represent linguistic information based on massive amounts of data. Unfortunately, for regional languages and dialects such resources are mostly not available. However, deep acoustic models might have learned linguistic information that transfers to low-resource languages. In this study, we evaluate whether this is the case through the task of distinguishing low-resource (Dutch) regional varieties. By extracting embeddings from the hidden layers of various wav2vec 2.0 models (including new models which are pre-trained and/or fine-tuned on Dutch) and using dynamic time warping, we compute pairwise pronunciation differences averaged over 10 words for over 100 individual dialects from four (regional) languages. We then cluster the resulting difference matrix in four groups and compare these to a gold standard, and a partitioning on the basis of comparing phonetic transcriptions. Our results show that acoustic models outperform the (traditional) transcription-based approach without requiring phonetic transcriptions, with the best performance achieved by the multilingual XLSR-53 model fine-tuned on Dutch. On the basis of only six seconds of speech, the resulting clustering closely matches the gold standard.

## Citation

```bibtex
@misc{bartelds-2022-quantifying,
  title = {{Quantifying Language Variation Acoustically with Few Resources}},
  author = {Bartelds, Martijn and Wieling, Martijn},
  year = {2022},
  publisher = {arXiv},
  url = {https://arxiv.org/abs/2205.02694},
  doi = {10.48550/ARXIV.2205.02694}
}
```

## Installation

```bash
git clone https://github.com/Bartelds/language-variation.git
cd language-variation
pip install -r requirements.txt
```

The clustering method uses the [L04 package](http://www.let.rug.nl/kleiweg/L04/index.html):
```bash
bash install_l04.sh
```

## Data

The recordings are obtained from the Goeman-Taeldeman-Van Reenen-Project held at the Meertens Institute in The Netherlands.
The collection identifier is `Collectie van het Meertens Instituut, nummer 2006, audiocollectie Goeman-Taeldeman-Van Reenen-project (1979-1996)`.

The set of recordings used in this study is available in `wav`.

## Usage

To compute embeddings from the hidden Transformer layers of the three fine-tuned deep acoustic wav2vec 2.0 large models, and evaluate the clustering results to the gold standard partitioning, please use: 

```bash
cd scripts
bash language_variation.sh
```

Different wav2vec 2.0 models can be used by changing the `MODEL_NAMES` in this script.

You can use any of the wav2vec 2.0 models available on the [Hugging Face Hub](https://huggingface.co/models?search=wav2vec2) ???? .

The Dutch wav2vec 2.0 models can be found at the [GroNLP organization](https://huggingface.co/GroNLP) on the Hugging Face Hub:
- [w2v2-nl](https://huggingface.co/GroNLP/wav2vec2-dutch-large-ft-cgn)
- [XLSR-nl](https://huggingface.co/GroNLP/wav2vec2-large-xlsr-53-ft-cgn)
