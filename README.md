# Quantifying Language Variation Acoustically with Few Resources
Code associated with the paper: Quantifying Language Variation Acoustically with Few Resources.

Accepted at NAACL 2022.

 > **Abstract**: Deep acoustic models represent linguistic information based on massive amounts of data.
Unfortunately, for regional languages and dialects such resources are mostly not available. However, deep acoustic models might have learned linguistic information that transfers to low-resource languages. In this study, we evaluate whether this is the case through the task of distinguishing low-resource (Dutch) regional varieties. By extracting embeddings from the hidden layers of various wav2vec 2.0 models (including a newly created Dutch model) and using dynamic time warping, we compute pairwise pronunciation differences averaged over 10 words for over 100 individual dialects from four (regional) languages. We then cluster the resulting difference matrix in four groups and compare these to a gold standard, and a partitioning on the basis of comparing phonetic transcriptions. Our results show that acoustic models outperform the (traditional) transcription-based approach without requiring phonetic transcriptions, with the best performance achieved by the multilingual XLSR-53 model fine-tuned on Dutch. On the basis of only six seconds of speech, the resulting clustering closely matches the gold standard.

## Installation

```bash
git clone https://github.com/Bartelds/language-variation.git
cd language-variation
pip install -r requirements.txt
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

You can use any of the wav2vec 2.0 models available on the [Hugging Face Hub](https://huggingface.co/models?search=wav2vec2) 🤗 .
