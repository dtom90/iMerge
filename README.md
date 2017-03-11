# iMerge
### Time Lapse Merging Program

## Inspiration
This project was inspired by the work of professional photographer Stephen Wilkes, who became famous for a project called “Day to Night” in which Wilkes would shoot about 1,400 photographs of a single scene over a period of approximately 15 hours. His final product would be a single photograph with certain areas of the image containing night versions of the scene, others containing day versions, and areas in between having a blending of the two.


## Introduction
The goal of this project was to implement a robust program which would take in as input a time-lapse series of photographs of a certain scene, and produce a single output image, which is a “merging” of the time-lapse image sequence. This merging consists of three main steps: alignment, foreground extraction, and merging. The alignment phase uses the gradient descent method known as Inverse Compositional Image Alignment, introduced by [2]. Once the images of the time-lapse sequence are aligned, these are passed into a foreground extraction algorithm, which uses the difference image between each image pair of the sequence (such that one image is the next image of the sequence after the other) and then uses the watershed algorithm [3] to extract the objects which change through the time-lapse image series. Finally, the merging step removes all the extracted objects and blends the background based on user selection. Then the extracted objects are all either inserted or removed, based on the user selection. This program allows for a user to easily and intuitively create a “merged” image from a given time-lapse sequence.
