import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import os

# Load the data
df = pd.read_csv("outputs/output.csv", usecols=range(7))  # Only load the 7 correct columns

# Programs and metrics to plot
programs = ["focus", "scan", "sort"]
metrics = ["pagefaults", "diskwrites", "diskreads"]

# Seaborn styling
sns.set(style="ticks")

# Create output directory if it doesn't exist
output_dir = "outputs"
os.makedirs(output_dir, exist_ok=True)

# Loop through each program
for program in programs:
    plt.figure(figsize=(18, 5))
    plt.suptitle(f"Performance Comparison on '{program}' Program", fontsize=16)

    # Filter data for the current program
    prog_df = df[df["program"] == program]

    # Create a subplot for each metric
    for i, metric in enumerate(metrics):
        plt.subplot(1, 3, i + 1)
        sns.lineplot(
            data=prog_df,
            x="nframes",
            y=metric,
            hue="algorithm",
            marker="o"
        )
        plt.title(metric.capitalize())
        plt.xlabel("Frames")
        plt.ylabel(metric.capitalize())

    plt.tight_layout(rect=[0, 0, 1, 0.95])

    # Save the figure
    filename = os.path.join(output_dir, f"{program}_performance.png")
    plt.savefig(filename)
    plt.close()  # Close the figure to free up memory
