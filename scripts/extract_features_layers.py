KNOWN_MODELS = {
    # Fine-tuned
    "wav2vec2-large-960h": "facebook/wav2vec2-large-960h",
    "wav2vec2-large-nl-ft-cgn": "GroNLP/wav2vec2-dutch-large-ft-cgn",
    "wav2vec2-large-xlsr-53-ft-cgn": "GroNLP/wav2vec2-large-xlsr-53-ft-cgn"
}


def load_wav2vec2_featurizer(model, layer=None):
    """
    Loads Wav2Vec2 featurization pipeline and returns it as a function.

    Featurizer returns a list with all hidden layer representations if "layer" argument is None.
    Otherwise, only returns the specified layer representations.
    """
    from transformers.models.wav2vec2 import Wav2Vec2Model
    import soundfile as sf
    import torch
    import numpy as np

    model_name_or_path = KNOWN_MODELS.get(model, model)
    model_kwargs = {}
    if layer is not None:
        model_kwargs["num_hidden_layers"] = layer if layer > 0 else 0
    model = Wav2Vec2Model.from_pretrained(model_name_or_path, **model_kwargs)
    model.eval()
    if torch.cuda.is_available():
        model.cuda()

    @torch.no_grad()
    def _featurize(path):
        input_values, rate = sf.read(path, dtype=np.float32)
        assert rate == 16_000
        input_values = torch.from_numpy(input_values).unsqueeze(0)
        if torch.cuda.is_available():
            input_values = input_values.cuda()

        if layer is None:
            hidden_states = model(input_values, output_hidden_states=True).hidden_states
            hidden_states = [s.squeeze(0).cpu().numpy() for s in hidden_states]
            return hidden_states

        if layer >= 0:
            hidden_state = model(input_values).last_hidden_state.squeeze(0).cpu().numpy()
        else:
            hidden_state = model.feature_extractor(input_values)
            hidden_state = hidden_state.transpose(1, 2)
            if layer == -1:
                hidden_state = model.feature_projection(hidden_state)
            hidden_state = hidden_state.squeeze(0).cpu().numpy()

        return hidden_state

    return _featurize


def main():
    import os
    from pathlib import Path
    from argparse import ArgumentParser

    import numpy as np
    from tqdm import tqdm

    parser = ArgumentParser(
        prog="Wav2Vec2 Featurizer",
        description="Runs full featurization of wav files for downstream usage.",
    )
    parser.add_argument("-i", "--input_dir", default="../wav", type=Path)
    parser.add_argument("-o", "--output_dir", default="../feats-seg/{model}/layer-{l}/{speaker}")
    parser.add_argument("-n", "--output_name", default="layer-{layer}.npy")
    parser.add_argument("-m", "--model", default="wav2vec2-large-960h")
    parser.add_argument("-f", "--force", action="store_true")
    args = parser.parse_args()

    # Check wav files in input directory
    wav_paths = list(args.input_dir.glob("*.wav"))
    if len(wav_paths) == 0:
        print(f"No wav files found in {args.input_dir}")
        exit(1)
    print(f"Featurizing {len(wav_paths):,} wav files")

    for l in range(1, 25):

        # Load acoustic model
        featurizer = load_wav2vec2_featurizer(args.model, layer=l)

        # Create features for each wav file
        for wav_path in tqdm(wav_paths, ncols=80):
            # Check output directory
            speaker = os.path.splitext(wav_path.name)[0]
            output_dir = Path(args.output_dir.format(model=args.model, speaker=speaker, l=l))

            # Extract features
            hidden_states = featurizer(wav_path)
            if l is not None:
                hidden_states = [hidden_states]

            # Save features
            os.makedirs(output_dir, exist_ok=True)
            for layer, hidden_state in enumerate(hidden_states, start=l or 0):
                feat_path = output_dir / args.output_name.format(layer=str(layer).zfill(2))
                np.save(feat_path, hidden_state)

            tqdm.write(str(output_dir))

        print("Done! Computed features for layer", l)


if __name__ == "__main__":
    main()
