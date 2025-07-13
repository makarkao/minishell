/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strjoin.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: makarkao <makarkao@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/14 22:25:24 by makarkao          #+#    #+#             */
/*   Updated: 2024/11/15 00:00:53 by makarkao         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strjoin(char const *s1, char const *s2)
{
	int		i;
	int		j;
	char	*strj;

	i = 0;
	j = 0;
	if (!s1)
		return ((char *)s2);
	if (!s2)
		return ((char *)s1);
	strj = malloc(sizeof(char) * (ft_strlen(s1) + ft_strlen(s2) + 1));
	if (!strj)
		return (NULL);
	while (s1[i])
	{
		strj[i] = s1[i];
		i++;
	}
	while (s2[j])
	{
		strj[i + j] = s2[j];
		j++;
	}
	strj[i + j] = '\0';
	return (strj);
}
