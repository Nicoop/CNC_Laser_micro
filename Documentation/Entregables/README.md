# LUDUS Academic Journal - LaTeX Template (English)

**Official LaTeX template for submissions to LUDUS Academic Journal**

---

## 📋 About LUDUS Academic Journal

**LUDUS Academic Journal** (ISSN: 3052-7597) is an international peer-reviewed journal dedicated to game studies, covering game design, game technology, game art, serious games, and related interdisciplinary research.

- **Journal Website**: [https://ludusofficial.site/](https://ludusofficial.site/)
- **Submission Email**: ludus.mag.info@gmail.com
- **Review Process**: Single-blind peer review
- **Review Timeline**: First decision within 30 working days

---

## 🚀 Quick Start

### 1. Open This Template in Overleaf

Click **"Open as Template"** in Overleaf Gallery, or upload this project to your Overleaf account.

### 2. Choose Article Type and Theme

Edit the first line of `main.tex`:

```latex
\documentclass[blue,fullpaper]{ludusofficial}
```

**Theme Colors** (choose one):
- `blue` (default) 🔵
- `red` 🔴  
- `green` 🟢
- `purple` 🟣
- `orange` 🟠
- `cyan` 🔵

**Article Types** (choose one):
- `fullpaper` - Full Paper (10-14 pages)
- `letter` - Letter/Poster (4 pages)
- `survey` - Survey Paper (15-20 pages)
- `shortpaper` - Short Paper (6-8 pages)
- `artpaper` - Art Paper (6-14 pages, may include interactive attachments)

### 3. Edit Metadata

Update the document metadata in `main.tex`:

```latex
\journalname{LUDUS}
\journalsubtitle{International Journal of Game Studies}
\conferencename{LUDUS 2026}  % Optional
\publicationyear{2026}
\articledoi{10.1234/ludus.2026.xxx}  % Leave empty if not assigned yet

\title{Your Article Title Here}
\shorttitle{Short Title}  % For page headers
\shortauthor{Smith \& Doe}  % For page headers

\author{%
    \textbf{Your Name}\textsuperscript{1}, \textbf{Co-author Name}\textsuperscript{2}\\[0.2cm]
    \small\textsuperscript{1}Your Institution, Country\\
    \small\textsuperscript{2}Co-author Institution, Country\\
    \small Corresponding: your.email@institution.edu
}
```

### 4. Write Your Abstract and Keywords

```latex
\begin{abstract}
Write your abstract here (200-500 words recommended).
\end{abstract}

\keywords{keyword1; keyword2; keyword3; keyword4}
```

### 5. Compile

This template works with:
- **XeLaTeX** (recommended) - supports OpenType fonts
- **PDFLaTeX** - uses standard LaTeX fonts

In Overleaf: Select **"XeLaTeX"** or **"PDFLaTeX"** from the compiler menu and click **"Recompile"**.

---

## 📄 Template Files

- **`main.tex`** - Main document (edit this file)
- **`ludusofficial.cls`** - Template class file (do not modify)
- **`logo.png`** - LUDUS journal logo (do not remove)
- **`references.bib`** - Bibliography database (add your references here)
- **`README.md`** - This file

---

## ✏️ Writing Your Paper

### Sections

```latex
\section{Introduction}
Your introduction text...

\subsection{Background}
Subsection content...

\subsubsection{Details}
Subsubsection content...
```

### Figures

**Single-column figure:**
```latex
\begin{figure}[htbp]
    \centering
    \includegraphics[width=\columnwidth]{your-image.pdf}
    \caption{Your caption here}
    \label{fig:example}
\end{figure}
```

**Full-width figure:**
```latex
\begin{figure*}[t]
    \centering
    \includegraphics[width=0.9\textwidth]{wide-image.pdf}
    \caption{Wide figure caption}
    \label{fig:wide}
\end{figure*}
```

### Tables

```latex
\begin{table}[htbp]
    \centering
    \caption{Table caption}
    \label{tab:example}
    \small
    \begin{tabular}{lcc}
        \hline
        \textbf{Item} & \textbf{Value 1} & \textbf{Value 2} \\
        \hline
        Row 1 & Data 1 & Data 2 \\
        Row 2 & Data 3 & Data 4 \\
        \hline
    \end{tabular}
\end{table}
```

### Mathematics

**Inline math:**
```latex
The equation $E = mc^2$ demonstrates...
```

**Display math:**
```latex
\[
\int_0^\infty e^{-x^2} dx = \frac{\sqrt{\pi}}{2}
\]
```

**Numbered equations:**
```latex
\begin{equation}
    F = ma
    \label{eq:newton}
\end{equation}

See Equation~\ref{eq:newton}...
```

### Citations and References

Add your references to `references.bib`, then cite them:

```latex
According to \cite{author2024}...
```

At the end of your document:

```latex
\bibliographystyle{ACM-Reference-Format}
\bibliography{references}
```

Or use the built-in bibliography environment:

```latex
\begin{thebibliography}{99}
\bibitem{ref1} Author, A. (2024). Title. \textit{Journal}, 1(1), 1-10.
\end{thebibliography}
```

### Acknowledgments

```latex
\begin{acknowledgments}
We thank...
\end{acknowledgments}
```

---

## 📏 Page Limits by Article Type

- **Full Paper**: 10-14 pages
- **Letter/Poster**: 4 pages
- **Survey**: 15-20 pages
- **Short Paper**: 6-8 pages
- **Art Paper**: 6-14 pages (may include supplementary materials)

Over-page fee: $100 USD per page over the limit.

---

## 📤 Submission Guidelines

### Required Files

When submitting to LUDUS, package these files:

1. **Main LaTeX file** (`main.tex` or `yourname.tex`)
2. **Class file** (`ludusofficial.cls`)
3. **Logo** (`logo.png`)
4. **Compiled bibliography** (`.bbl` file) - Important for Overleaf/arXiv submissions
5. **All figures** (`.pdf`, `.png`, `.jpg`)
6. **Final PDF** (compiled output)

### How to Package in Overleaf

1. Click **Menu** (top left)
2. Scroll to **Download**
3. Select **"Source"** 
4. This downloads a `.zip` file with all source files
5. Rename the `.zip` to: `YourLastName_LUDUS_2026.zip`

### Submit Via

Send your `.zip` file to: **ludus.mag.info@gmail.com**

Include in email:
- Article title
- Author names
- Article type
- Brief cover letter (optional, use our template if needed)

---

## 🎨 Customization Tips

### Changing Body Font Size

The default body text is ~10.5-11pt. To adjust, edit `ludusofficial.cls` line 11:

```latex
\LoadClass[10pt,a4paper,twocolumn,twoside]{article}
% Change to 9pt, 10pt, 11pt, or 12pt
```

### Adjusting Margins

Edit the `geometry` package settings in `ludusofficial.cls` (~line 70-80).

---

## ❓ Frequently Asked Questions

**Q: Can I use PDFLaTeX instead of XeLaTeX?**  
A: Yes. The template works with both compilers.

**Q: My figures are not showing up.**  
A: Make sure image files are uploaded to Overleaf and the file paths are correct. Use `\includegraphics{filename.pdf}` without the path if the file is in the same folder.

**Q: How do I add line numbers for review?**  
A: Add `\usepackage{lineno}` and `\linenumbers` in the preamble.

**Q: Can I modify the template colors?**  
A: Yes. Edit the color definitions in `ludusofficial.cls` (search for `\definecolor{ludusblue}`, etc.).

**Q: Where can I find the submission guidelines?**  
A: Visit [https://ludusofficial.site/](https://ludusofficial.site/) or contact ludus.mag.info@gmail.com.

---

## 📞 Support

- **Email**: ludus.mag.info@gmail.com
- **Website**: [https://ludusofficial.site/](https://ludusofficial.site/)
- **ISSN**: 3052-7597

---

## 📜 License

This template is provided by LUDUS Academic Journal for use by authors submitting to the journal. You may freely modify and use this template for your submissions.

---

**Good luck with your submission!** 🎓
